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
// File: ilgen.cpp
//
// Routines for generating il for a method
// ===========================================================================

#include "stdafx.h"

#include "compiler.h"

#if !DEBUG 
#define dumpAllBlocks()
#define dumpAllBlocksContent()
#endif

// We want to find out where we leak our temporaries...
#if DEBUG
#define allocTemporary(t,k) allocTemporary(t,k, __FILE__, __LINE__)
#define allocTemporaryRef(t,k) allocTemporaryRef(t,k, __FILE__, __LINE__)
#endif

#define FREETEMP(temp) \
if (temp) { \
    freeTemporary(temp); \
    (temp) = NULL; \
}

// This is a macro instead of a function so that we can capture at what line it
#define STOREINTEMP(type, kind) \
    (storeLocal(allocTemporary(type, kind)))


// array of ilcodes...
const REFENCODING ILGENREC::ILcodes [] = {
#define OPDEF(id, name, pop, push, operand, type, len, b1, b2, cf) {b1, b2} ,
#include "opcode.def"
#undef OPDEF
};

// constructor, just use the local allocator...
ILGENREC::ILGENREC()
{
    allocator = &(compiler()->localSymAlloc);
}

// the method visible to the workld...
void ILGENREC::compile(METHSYM * method, METHINFO * info, EXPR * tree) 
{
    TimerStart(TIME_ILGEN);

#if DEBUG
    if (compiler()->GetRegDWORD("SmallBlock")) {
        smallBlock = true;
    } else {
        smallBlock = false;
    }
#endif

    // save our context...
    this->method = method;
    this->cls = method->parent->asAGGSYM();
    this->info = info;

    initFirstBB();

    // initialize temporaries and locals and params
    temporaries = NULL;
    localScope = getLocalScope();
    if (localScope) {
        permLocalCount = assignLocals(0, localScope);
    } else {
        permLocalCount = 0;
    }
    assignParams();

    globalFieldCount = 0;
    curStack = 0;
    maxStack = 0;
    handlers = NULL;
    lastHandler = NULL;
    ehCount = 0;
    returnLocation = NULL;
    blockedLeave = 0;
    origException = NULL;
    curDebugInfo = NULL;
    if (method->retType != compiler()->symmgr.GetVoid() && (info->hasRetAsLeave || !compiler()->options.m_fOPTIMIZATIONS)) {
        retTemp = allocTemporary(method->retType, TK_RETURN);
    } else {
        retTemp = NULL;
    }

    lastPos.iLength = 0;
    lastNode = NULL;


    // generate the prologue

    genPrologue();

    // generate the code
    genBlock(tree->asBLOCK());


    // do the COM+ magic to emit the method:
    SETLOCATIONSTAGE(EMITIL);

    unsigned codeSize = getFinalCodeSize();

    // make sure no temps leaked
#if DEBUG
    verifyAllTempsFree();
#endif

    COR_ILMETHOD_FAT fatHeader;
	if (!compiler()->options.m_fUNSAFE) {
        fatHeader.SetFlags(CorILMethod_InitLocals);
    } else {
        fatHeader.SetFlags(0);
    }
    fatHeader.SetMaxStack(maxStack);
    fatHeader.SetCodeSize(codeSize);
    if (permLocalCount || temporaries) {
        fatHeader.SetLocalVarSigTok(computeLocalSignature());
    } else {
        fatHeader.SetLocalVarSigTok(NULL);
    }

    COR_ILMETHOD_SECT_EH_CLAUSE_FAT * clauses = (COR_ILMETHOD_SECT_EH_CLAUSE_FAT *) _alloca(ehCount * sizeof(COR_ILMETHOD_SECT_EH_CLAUSE_FAT));
    copyHandlers(clauses);

    bool moreSections = ehCount != 0;
    unsigned alignmentJunk;
    if (moreSections) {
        alignmentJunk = ((codeSize + 3) & ~3) - codeSize;
        codeSize += alignmentJunk;
    } else {
        alignmentJunk = 0;
    }

    unsigned headerSize = COR_ILMETHOD::Size(&fatHeader, moreSections);
    unsigned ehSize = COR_ILMETHOD_SECT_EH::Size(ehCount, codeSize);
    unsigned totalSize = headerSize + codeSize + ehSize;
    bool align = headerSize != 1;

    BYTE * buffer = (BYTE*) compiler()->emitter.EmitMethodRVA(method, totalSize, align ? 4 : 1);

    emitDebugInfo(codeSize - alignmentJunk);

    BYTE * bufferBeg = buffer;

#if DEBUG
    BYTE * endBuffer = &buffer[totalSize];
#endif

    buffer += COR_ILMETHOD::Emit(headerSize, &fatHeader, moreSections, buffer);

    buffer = copyCode(buffer, (unsigned)(buffer - bufferBeg));

    buffer += alignmentJunk;

    buffer += COR_ILMETHOD_SECT_EH::Emit(ehSize, ehCount, clauses, false, buffer);

    ASSERT(buffer == endBuffer);

    // and we are done...
    TimerStop(TIME_ILGEN);
}

// terminate the current bb with the given exit code & jump destination
void ILGENREC::endBB(ILCODE exitIL, BBLOCK * jumpDest)
{
    // remember, if currently being emitted to, the bb has its exit set to CEE_ILLEGAL
    ASSERT(inlineBB.exitIL == CEE_ILLEGAL);

    inlineBB.exitIL = exitIL;
#if DEBUG
    if (jumpDest) {
        inlineBB.jumpDest = jumpDest;
    } else {
        memset( &inlineBB.jumpDest, 0xCCCCCCCC, sizeof(BBLOCK*));
    }
#else
    inlineBB.jumpDest = jumpDest;
#endif

    // copy the code into a more permanent place...
    inlineBB.curLen = inlineBB.code - reusableBuffer;
    BYTE * newBuffer = (BYTE*) allocator->Alloc(inlineBB.curLen + BB_TOPOFF);
    memcpy(newBuffer, reusableBuffer, inlineBB.curLen);
    inlineBB.code = newBuffer;

    // and finally code the bb info to its permanent mapping
    memcpy(currentBB, &inlineBB, sizeof(BBLOCK));
}


// initialzie the inline bb.  we only initialize the fields we know we
// are going to read before ending the bb.
inline void ILGENREC::initInlineBB()
{
    inlineBB.code = reusableBuffer;
    inlineBB.debugInfo = NULL;
    inlineBB.v.mayRemove = false;
    inlineBB.v.startsTry = false;
    inlineBB.v.endsFinally = false;
    inlineBB.v.startsCatchOrFinally = false;
    inlineBB.v.jumpIntoTry = false;
    inlineBB.v.gotoBlocked = false;
#if DEBUG
    inlineBB.exitIL = CEE_ILLEGAL;
    inlineBB.startOffset = (unsigned) -1;
    inlineBB.sym = NULL;
    inlineBB.curLen = 0xffffffff;
#endif
}


// create a new basic block and maybe make it the current one so that
// we can emit to it.
BBLOCK * ILGENREC::createNewBB(bool makeCurrent)
{
    BBLOCK * rval = (BBLOCK*) allocator->Alloc(sizeof(BBLOCK));
    if (makeCurrent) {
        ASSERT(inlineBB.exitIL != CEE_ILLEGAL);

        initInlineBB();
        currentBB = rval;
    }

    return rval;
}

// close the previuos bb and start a new one, either the one provided
// by next, or a brand new one...
// Return the now current BB
BBLOCK * ILGENREC::startNewBB(BBLOCK * next, ILCODE exitIL, BBLOCK * jumpDest)
{
    endBB(exitIL, jumpDest);
    if (next) {
        currentBB->next = next;
        initInlineBB();
        currentBB = next;
    } else {
        BBLOCK * prev = currentBB;
        createNewBB(true);
        prev->next = currentBB;
    }
    return currentBB;
}

// this gets called if the inline bb gets too full, basically, we just
// didn't want to inline one extra param (the null)
void ILGENREC::flushBB()
{
    startNewBB(NULL);
}

// initialize the first bb before generating code
void ILGENREC::initFirstBB()
{
#if DEBUG
    inlineBB.exitIL = cee_next;
#endif
    firstBB = currentBB = createNewBB(true);
}

// emit a single opcode to e given buffer.  advance the buffer by the size of the opcode
__forceinline void ILGENREC::putOpcode(BYTE ** buffer, ILCODE opcode)
{
    ASSERT(opcode != CEE_ILLEGAL && opcode != CEE_UNUSED1 && opcode < cee_last);
    REFENCODING ref = ILcodes[opcode];
    if (ref.b1 != 0xFF) {
        ASSERT(ILcodesSize[opcode] == 2);
        *(REFENCODING*)(*buffer) = ref;
        (*buffer) += 2;
    } else {
        ASSERT(ILcodesSize[opcode] == 1);
        (**buffer) = ref.b2;
        (*buffer)++;
    }
}

// emit, but to the default buffer from the inline bb
__forceinline void ILGENREC::putOpcode(ILCODE opcode)
{
    if (inlineBB.code > (reusableBuffer + BBSIZE)) {
        flushBB();
    }
#if DEBUG
    if (smallBlock) {
        startNewBB(NULL);
    }
#endif
    putOpcode(&(inlineBB.code), opcode);
    curStack += ILStackOps[opcode];
    markStackMax();
}

// write a given value to the ilcode stream
void ILGENREC::putWORD(WORD w)
{
    SET_UNALIGNED_VAL16(inlineBB.code, w);
    inlineBB.code += sizeof(WORD);
}

void ILGENREC::putDWORD(DWORD dw)
{
    SET_UNALIGNED_VAL32(inlineBB.code, dw);
    inlineBB.code += sizeof(DWORD);
}

void ILGENREC::putCHAR(char c)
{
    (*(char*)(inlineBB.code)) = c;
    inlineBB.code += sizeof(char);
}

void ILGENREC::putQWORD(__int64 * qv)
{
    SET_UNALIGNED_VAL64(inlineBB.code, *qv);
    inlineBB.code += sizeof(__int64);
}

// return the scope just below the outermose param scope, if any
SCOPESYM * ILGENREC::getLocalScope()
{

    SYM * current = info->outerScope->firstChild;
    while (current) {
        if (current->kind == SK_SCOPESYM && !(current->asSCOPESYM()->scopeFlags & SF_ARGSCOPE)) {
            return current->asSCOPESYM();
        }
        current = current->nextChild;
    };

    return NULL;
}


void ILGENREC::initLocal(PSLOT slot, TYPESYM * type)
{
    if (!type) {
        type = slot->type;
        ASSERT(type);
    }
    
    if (type->fundType() != FT_STRUCT) { // || type->asAGGSYM()->iPredef == PT_INTPTR) {
        genZero(NULL, type);
        dumpLocal(slot, true);
    } else {
        genSlotAddress(slot);
        putOpcode(CEE_INITOBJ);
        emitTypeToken(type);
    }

}

void ILGENREC::preInitLocals(SCOPESYM * scope)
{

	// all locals are automatically preinitialized by the jit

    PSLOT thisPointer = compiler()->funcBRec.getThisPointerSlot();

    if (thisPointer && thisPointer->needsPreInit) {
        initLocal(thisPointer);
        thisPointer->needsPreInit = false;
    }
}

bool ILGENREC::isExprOptimizedAway(EXPR * tree)
{
AGAIN:
    switch (tree->kind) {       
    case EK_LOCAL:
        return tree->asLOCAL()->local->slot.type == NULL;
    case EK_FIELD:
        tree = tree->asFIELD()->object;
        if (tree) goto AGAIN;
        return false;
    default:
        return false;
    }
}

// assign slots to all locals for a given scope and starting with
// a privided index.  returns the next available slot
unsigned ILGENREC::assignLocals(unsigned startWith, SCOPESYM * scope)
{
    unsigned startLocation = startWith;
    LOCVARSYM * loc;

    SYM * current = scope->firstChild;
    while (current) {
        switch (current->kind) {
        case SK_LOCVARSYM:
            loc = current->asLOCVARSYM();
            if ((!compiler()->options.m_fOPTIMIZATIONS || compiler()->funcBRec.localIsUsed(loc))
                && !loc->isConst) {
                loc->slot.ilSlot = startLocation++;
                loc->slot.type = loc->type;
                loc->slot.isParam = false;
                loc->firstUsed.SetEmpty();
            } else {
                loc->slot.type = NULL;
            }
            break;
        case SK_SCOPESYM:
            startLocation = assignLocals(startLocation, current->asSCOPESYM());
            break;
        default:
            ASSERT(current->kind == SK_CACHESYM || current->kind == SK_LABELSYM);
        }
        current = current->nextChild;
    }

    return startLocation;
}

#define OLD_USED_SLOT_INDEX ((unsigned) 0xFFFFFFFF)

void ILGENREC::assignEncLocals(ENCLocal *oldLocals, unsigned *newLocalSlot, SCOPESYM * scope)
{
    LOCVARSYM * loc;

    SYM * current = scope->firstChild;
    while (current) {
        switch (current->kind) {
        case SK_LOCVARSYM:
            loc = current->asLOCVARSYM();
            if ((!compiler()->options.m_fOPTIMIZATIONS || compiler()->funcBRec.localIsUsed(loc))
                && !loc->isConst) {

                //
                // find an old local with same name and type
                // which hasn't been mapped to a new local yet
                //
                ENCLocal *local = oldLocals;
                while (local && (local->name != loc->name || 
                                 local->type != loc->type ||
                                 OLD_USED_SLOT_INDEX == local->slot)) {
                    local = local->next;
                }

                if (local) {
                    //
                    // found an old local we'll map too
                    //
                    loc->slot.ilSlot = local->slot;
                    local->slot = OLD_USED_SLOT_INDEX;
                } else {
                    //
                    // didn't find an old local, allocate a new one at the
                    // end of the list of old locals and temps
                    //
                    loc->slot.ilSlot = *newLocalSlot;
                    *newLocalSlot += 1;
                }
                loc->slot.type = loc->type;
                loc->slot.isParam = false;
                loc->firstUsed.SetEmpty();
            } else {
                loc->slot.type = NULL;
            }
            break;

        case SK_SCOPESYM:
            assignEncLocals(oldLocals, newLocalSlot, current->asSCOPESYM());
            break;

        default:
            ASSERT(current->kind == SK_CACHESYM || current->kind == SK_LABELSYM);
        }
        current = current->nextChild;
    }
}

void ILGENREC::assignEncLocals(ENCLocal *oldLocals)
{
    unsigned numberOfOldLocals;
    unsigned newLocalsSlot;
    unsigned totalTemporaries = 0;
    ENCLocal *locals;

    //
    // count number of old locals and temps
    //
    numberOfOldLocals = 0;
    for (locals = oldLocals; locals; locals = locals->next) {
        numberOfOldLocals += 1;
    }
    newLocalsSlot = numberOfOldLocals;

    //
    // allocate named locals
    //
    // attempt to match new locals with old locals
    // if not found then new local gets a slot after the
    // last old temporary.
    //
    if (NULL != getLocalScope()) {
        assignEncLocals(oldLocals, &newLocalsSlot, getLocalScope());
    }

    //
    // mark all old named locals which aren't getting reused as
    // being deleted temporaries
    //              &&
    // allocate space for the old unnamed temporaries
    //
    for (locals = oldLocals; locals; locals = locals->next) {
        if (OLD_USED_SLOT_INDEX != locals->slot) {
            //
            // found a slot which wasn't reused
            // mark it as being deleted
            //
            LOCSLOTINFO *slot;
            if (locals->name && *locals->name->text) {
                //
                // unused named locals are marked as deleted
                //
                slot = allocTemporary(locals->type, TK_DELETEDENCVARIABLE);
                slot->isDeleted = true;
            } else {
                 slot = allocTemporary(locals->type, locals->tempKind);
                 slot->canBeReusedInEnC = true;
            }
            slot->ilSlot = locals->slot;
            totalTemporaries += 1;
        }
    }

    //
    // mark all non-deleted temporaries as being not-taken
    // need to do this after they've all been allocated so 
    // that they don't get reused during the allocation
    //
    {
        for (TEMPBUCKET * bucket = temporaries; bucket; bucket = bucket->next) {
            for (int i = 0; i < TEMPBUCKETSIZE; i += 1) {
                if (!bucket->slots[i].isDeleted) {
                    bucket->slots[i].isTaken = false;
                }
            }
        }
    }

    //
    // set the ILGENREC state correctly ...
    //
    permLocalCount = (newLocalsSlot - totalTemporaries);

    return;
}

// store in array the types of all locals in scope.  return the array
// sufficiently advanced for the next local.  this assumes that the
// array was allocated of a sufficient size before calling this
// function.
TYPESYM ** ILGENREC::getLocalTypes(TYPESYM ** array, SCOPESYM * scope)
{
    for  (SYM * current = scope->firstChild; current; current = current->nextChild) {
        switch (current->kind) {
        case SK_LOCVARSYM:
            if (!current->asLOCVARSYM()->slot.type) continue;
            if (current->asLOCVARSYM()->isConst) continue;
            ASSERT(!array[current->asLOCVARSYM()->slot.ilSlot]);
            if (current->asLOCVARSYM()->slot.isPinned) {
                array[current->asLOCVARSYM()->slot.ilSlot] = compiler()->symmgr.GetPinnedType(current->asLOCVARSYM()->slot.type);
            } else {
                array[current->asLOCVARSYM()->slot.ilSlot] = current->asLOCVARSYM()->slot.type;
            }
            break;
        case SK_SCOPESYM:
            getLocalTypes(array, current->asSCOPESYM());
            break;
        default:
            break;
        }
    }

    return array;
}

// assign slots to all parameters of the method being compiled...
void ILGENREC::assignParams()
{
    unsigned curSlot = 0;

    SYM * current = info->outerScope->firstChild;

    while (current) {
        if (current->kind == SK_LOCVARSYM) {
            LOCVARSYM * loc = current->asLOCVARSYM();
            loc->slot.type = loc->type;
            loc->slot.isParam = true;
            loc->slot.ilSlot = curSlot++;
        }
        current = current->nextChild;
    }
}


// compute the signature of all locals, explicit and temporaries
mdToken ILGENREC::computeLocalSignature()
{
    // count the temps:
    unsigned temps = 0;
    TEMPBUCKET *bucket;
    for(bucket = temporaries; bucket; bucket = bucket->next) {
        for (int i = 0; i < TEMPBUCKETSIZE  && bucket->slots[i].type; ++temps, ++i);
    }

    int total = permLocalCount + temps;

    // allocate enough space
    TYPESYM ** types = (TYPESYM**) _alloca(sizeof(TYPESYM*) * total);
#if DEBUG
    memset(types, 0, sizeof(TYPESYM*) * total);
#endif

    // collect the local types
    TYPESYM ** current;
    if (localScope) {
        current = getLocalTypes(types, localScope);
    } else {
        current = types;
    }

    // and the temporary types
    for(bucket = temporaries; bucket; bucket = bucket->next) {
        for (int i = 0; i < TEMPBUCKETSIZE && bucket->slots[i].type; i++) {
            ASSERT(bucket->slots[i].ilSlot < (unsigned)total);
            ASSERT(!types[bucket->slots[i].ilSlot]);
            types[bucket->slots[i].ilSlot] = bucket->slots[i].type;
        }
    }

#if DEBUG
    for (int i = 0; i < total; i += 1) {
        ASSERT(types[i]);
    }
#endif

    return compiler()->emitter.GetSignatureRef(total, types);
}


void ILGENREC::handleReturn(bool addDebugInfo)
{
    if (returnLocation && !returnHandled && !retTemp) {
        startNewBB(returnLocation);
        returnHandled = true;
    }
    if (method->retType != compiler()->symmgr.GetVoid()) curStack --;

    if (addDebugInfo) {
        openDebugInfo(getCloseIndex(), 0);
    }

    if (compiler()->options.m_fEMITDEBUGINFO && !compiler()->options.m_fOPTIMIZATIONS && maxStack == 0) { // This genrates a few false positive's but that's OK
        // Emit a NOP so that debug-info has someplace to step into.
        putOpcode(CEE_NOP);
    }
    putOpcode(CEE_RET);
}


void ILGENREC::genPrologue()
{


    // preinit out params:

    if (info->preInitsNeeded) {
        SCOPESYM * localScope = getLocalScope();
        if (localScope) {
            preInitLocals(localScope);
        }
    }

    // switch hashtables:

    BBLOCK * contBlock = NULL;

    EXPRLOOP(info->switchList, switchStmt)        

        EXPRSWITCH * expr = switchStmt->asSWITCH();
        expr->hashtableToken = compiler()->emitter.GetGlobalFieldDef(method, ++globalFieldCount, compiler()->symmgr.GetPredefType(PT_HASHTABLE));
    
        if (!contBlock) {
            contBlock = createNewBB();
        }

        putOpcode(CEE_VOLATILE);
        putOpcode(CEE_LDSFLD);
        putDWORD(expr->hashtableToken);
        startNewBB(NULL, CEE_BRTRUE, contBlock);

        genIntConstant(expr->labelCount * 2);
        genFloatConstant(0.5);
        putOpcode(CEE_NEWOBJ);
        emitMethodToken(info->hTableCreate);
        curStack-=3;

        int labelCount = expr->labelCount;
        if (expr->flags & EXF_HASDEFAULT) labelCount--;

        EXPRSWITCHLABEL ** start = expr->labels;
        EXPRSWITCHLABEL ** end = start + labelCount;

        int caseNumber = 0;

        while (start != end) {
            if (!start[0]->key->asCONSTANT()->isNull()) {
                putOpcode(CEE_DUP); // dup the hashtable...
                genString(start[0]->key->asCONSTANT()->getSVal());
                genIntConstant(caseNumber++);
                putOpcode(CEE_BOX);
                emitTypeToken(compiler()->symmgr.GetPredefType(PT_INT));
                putOpcode(CEE_CALL);
                emitMethodToken(info->hTableAdd);
                curStack -= 3;
            }
            start ++;
        }

        putOpcode(CEE_VOLATILE);
        putOpcode(CEE_STSFLD);
        putDWORD(expr->hashtableToken);
         
    ENDLOOP;

    if (contBlock) {
        startNewBB(contBlock);
    }
}

// generate code for a block
void ILGENREC::genBlock(EXPRBLOCK * tree)
{
    bool emitDebugInfo = !!compiler()->options.m_fEMITDEBUGINFO;

    // record debug info for scopes, so we can emit that later.
    if (emitDebugInfo && tree->scopeSymbol) {
        tree->scopeSymbol->debugBlockStart = currentBB;
        tree->scopeSymbol->debugOffsetStart = getCOffset();
    }

    EXPRLOOP(tree->statements, stmt)
        genStatement(stmt);
    ENDLOOP;

    if (tree->flags & EXF_NEEDSRET) {
        handleReturn(true);
        startNewBB(NULL, CEE_RET);
        closeDebugInfo();
    }

    if (emitDebugInfo && tree->scopeSymbol) {
        tree->scopeSymbol->debugBlockEnd = currentBB;
        tree->scopeSymbol->debugOffsetEnd = getCOffset();
    }
}




long ILGENREC::getCloseIndex() 
{

    BLOCKNODE * block;

    if (!method) return -1;
    BASENODE * tree = method->getParseTree();
    if (!tree) return -1;

    switch (tree->kind) {
    case NK_ACCESSOR:
        return tree->asACCESSOR()->iClose;
    case NK_METHOD:
    case NK_CTOR:
    case NK_OPERATOR:
        block = tree->asANYMETHOD()->pBody;
        if (!block) return -1;
        return block->iClose;

    default:
        return -1;
    }
    
}


long ILGENREC::getOpenIndex()
{
    if (!method) return -1;
    BASENODE * tree = method->getParseTree();
    if (!tree) return -1;

    switch (tree->kind) {
    case NK_ACCESSOR:
        return tree->asACCESSOR()->iOpen;
    case NK_METHOD:
    case NK_CTOR:
    case NK_OPERATOR:
        return  tree->asANYMETHOD()->iOpen;

    default:
        return -1;
    }
}


void ILGENREC::openDebugInfo(long index, int flags)
{

    if (!compiler()->options.m_fEMITDEBUGINFO || (index == -1 && !(flags & EXF_NODEBUGINFO))) return;

    createNewDebugInfo();
    emitDebugDataPoint(index, flags);
}

void ILGENREC::openDebugInfo(BASENODE * tree, int flags)
{
    if (!compiler()->options.m_fEMITDEBUGINFO) return;

    createNewDebugInfo();
    emitDebugDataPoint(tree, flags);
}

void ILGENREC::createNewDebugInfo()
{

    ASSERT(!curDebugInfo);
    curDebugInfo = (DEBUGINFO*) allocator->Alloc(sizeof(DEBUGINFO));
    curDebugInfo->beginBlock = currentBB;
    curDebugInfo->begin.SetEmpty();
    curDebugInfo->end.SetEmpty();
    curDebugInfo->next = NULL;
    curDebugInfo->prev = inlineBB.debugInfo;
    if (inlineBB.debugInfo) {
        inlineBB.debugInfo->next = curDebugInfo;
    }
    inlineBB.debugInfo = curDebugInfo;
    curDebugInfo->beginOffset = (unsigned short) getCOffset();

}
// generate code for a statement, in retail, this does the work itself,
// in debug it calls a worker function and verifies that nothing was
// left on the stack...
#ifdef DEBUG
void ILGENREC::genStatement(EXPR * tree)
{
    genStatementInner(tree);
    ASSERT(curStack == 0);
}


void ILGENREC::genStatementInner(EXPR * tree)
#else
void ILGENREC::genStatement(EXPR * tree)
#endif
{

AGAIN:

    if (!tree) return;

    SETLOCATIONNODE(tree->tree);

    if (compiler()->options.m_fEMITDEBUGINFO) {
        switch (tree->kind) {
        case EK_BLOCK:
        case EK_LIST:
        case EK_SWITCHLABEL:
        case EK_TRY:
            break;
        default:
            openDebugInfo(tree->tree, tree->flags);
        }
    }

    switch (tree->kind) {
    case EK_STMTAS:
        genExpr(tree->asSTMTAS()->expression, NULL);
        break;
    case EK_RETURN:
        genReturn(tree->asRETURN());
        break;
    case EK_DECL:

        maybeEmitDebugLocalUsage(tree->tree, tree->asDECL()->sym);
        genExpr(tree->asDECL()->init, NULL);
        break;
    case EK_BLOCK:

        genBlock(tree->asBLOCK());

        break;
    case EK_ASSERT:
        genGoto(tree->asASSERT());
        break;
    case EK_GOTO:
        genGoto(tree->asGOTO());
        break;
    case EK_GOTOIF:
        genGotoIf(tree->asGOTOIF());
        break;
    case EK_SWITCHLABEL:
        genLabel(tree->asSWITCHLABEL());
        tree = tree->asSWITCHLABEL()->statements;
        goto AGAIN;
    case EK_LABEL:
        genLabel(tree->asLABEL());
        break;
    case EK_LIST:
        genStatement(tree->asBIN()->p1);
        tree = tree->asBIN()->p2;
        goto AGAIN;
    case EK_SWITCH:
        genSwitch(tree->asSWITCH());
        break;
    case EK_THROW:
        genThrow(tree->asTHROW());
        break;
    case EK_TRY:
        genTry(tree->asTRY());
        break;
    default:
        ASSERT(!"bad expr type");
        
    }

    closeDebugInfo();
    
}


void ILGENREC::closeDebugInfo() 
{

    if (!curDebugInfo) return;

#if DEBUG
    if (curDebugInfo->prev) {
        ASSERT(curDebugInfo->prev->next == curDebugInfo);
    }
#endif

    if (curDebugInfo->beginBlock == currentBB && curDebugInfo->beginOffset == getCOffset()) {
        if (curDebugInfo->prev) {
            curDebugInfo->prev->next = NULL;
        }
        if (curDebugInfo->beginBlock == currentBB) {
            ASSERT(inlineBB.debugInfo == curDebugInfo);
            inlineBB.debugInfo = curDebugInfo->prev;
        } else {
            ASSERT(curDebugInfo->beginBlock->debugInfo == curDebugInfo);
            curDebugInfo->beginBlock->debugInfo = curDebugInfo->prev;
        }
        curDebugInfo->beginBlock->debugInfo = curDebugInfo->prev;
        curDebugInfo = NULL;
        return;
    }
    curDebugInfo->endBlock = currentBB;
    curDebugInfo->endOffset = getCOffset();
    curDebugInfo = NULL;
}

__forceinline void ILGENREC::maybeEmitDebugLocalUsage(BASENODE * tree, LOCVARSYM * sym)
{
    if (compiler()->options.m_fEMITDEBUGINFO && sym->slot.type) {
        emitDebugLocalUsage(tree, sym);
    }
}

TOKENDATA __fastcall ILGENREC::getPosFromTree(BASENODE * tree)
{
    TOKENDATA td;
    td.iToken = TID_UNKNOWN;
    td.pName = NULL;
    td.iLength = -1;
    // This always return S_OK, so don't check return value
    method->getInputFile()->pData->GetExtentEx( tree, &td.posTokenStart, &td.posTokenStop, EF_SINGLESTMT);

    return td;
}

TOKENDATA __fastcall ILGENREC::getPosFromIndex(long index)
{
    TOKENDATA td;
    HRESULT hr = method->getInputFile()->pData->GetSingleTokenData(index, &td);
    if (FAILED(hr))
        memset(&td, 0, sizeof(TOKENDATA));
    return td;
}

void ILGENREC::emitDebugLocalUsage(BASENODE * tree, LOCVARSYM * sym)
{

    if (tree != lastNode) {
        lastNode = tree;
        lastPos = getPosFromTree(tree);
    }

    if (lastPos.posTokenStart < sym->firstUsed) {
        sym->firstUsed = lastPos.posTokenStart;
        sym->debugBlockFirstUsed = currentBB;
        sym->debugOffsetFirstUsed = getCOffset();
    }
}


void ILGENREC::emitDebugDataPoint(long index, int flags)
{

    ASSERT(curDebugInfo);

    if (index == -1) {
        ASSERT(flags & EXF_NODEBUGINFO);
        lastPos.posTokenStart.SetEmpty();
        lastPos.posTokenStop.SetEmpty();
        lastPos.iLength = 0;
        lastNode = NULL;
        setDebugDataPoint();
        return;
    }

    TOKENDATA pd = getPosFromIndex(index);

    if (pd.posTokenStop > lastPos.posTokenStop || lastPos.iLength == 0) {
        lastPos = pd;
        lastNode = NULL;
        if (flags & EXF_NODEBUGINFO) {
            lastPos.posTokenStop.SetEmpty();
        }
        setDebugDataPoint();
    }
}

void ILGENREC::emitDebugDataPoint(BASENODE * tree, int flags)
{

    ASSERT(curDebugInfo);

    if (!tree || (tree == lastNode && !(flags & EXF_NODEBUGINFO))) return;

    // Only set this if we don't have a statement already
    if (curDebugInfo->begin.IsEmpty() || curDebugInfo->end.IsEmpty() || curDebugInfo->begin == curDebugInfo->end) {
        lastNode = tree;
        lastPos = getPosFromTree(tree);
        if (flags & EXF_NODEBUGINFO) {
            lastPos.posTokenStop.SetEmpty();
            lastNode = NULL;
        }
        setDebugDataPoint();
    }
}


void ILGENREC::setDebugDataPoint()
{

    ASSERT(lastPos.iLength != 0 || (lastPos.posTokenStart.IsEmpty() && lastPos.posTokenStop.IsEmpty()));

    if (lastPos.posTokenStop > curDebugInfo->end) {
        curDebugInfo->end = lastPos.posTokenStop;
    } else if (curDebugInfo->begin.IsEmpty()) {
        ASSERT(curDebugInfo->end.IsEmpty());
        curDebugInfo->begin = lastPos.posTokenStart;
        curDebugInfo->end = lastPos.posTokenStop;
    } else if (lastPos.posTokenStart < curDebugInfo->begin) {
        curDebugInfo->begin = lastPos.posTokenStart;
    }
}

void ILGENREC::emitDebugInfo(unsigned codeSize)
{
    // Some methods (e.g., implicitly created constructors) shouldn't have
    // debug info emitted.
    if (info->noDebugInfo)
        return;

    // Count how many sequence points we have.
    int count = 0;
    BBLOCK *current;
    for (current = firstBB; current; current = current->next) {
        for (DEBUGINFO * debInfo = current->debugInfo; debInfo; debInfo = debInfo->prev) {
            count ++;
        }
    }

    //If none, don't emit any debug info.
    if (count == 0)
        return;

    // Begin emitting method debug info.
    compiler()->emitter.EmitDebugMethodInfoStart(method);


    // add one, for the possibility that we may have prologue code
    count++;

    // allocate arrays for offsets, lines, cols.
    unsigned int * offsets;
    unsigned int * lines;
    unsigned int * cols;
    unsigned int * endLines;
    unsigned int * endCols;

    unsigned int allocSize = count * sizeof(int);
    
    if (allocSize > (500000 / 5)) { 
        // if we alloc more than 1/2 meg (stacksize is 1 meg by default) then don't use stack
        offsets = (unsigned int *) allocator->Alloc(allocSize);
        lines = (unsigned int *) allocator->Alloc(allocSize);
        cols = (unsigned int *) allocator->Alloc(allocSize);
        endLines = (unsigned int *) allocator->Alloc(allocSize);
        endCols = (unsigned int *) allocator->Alloc(allocSize);        
    } else {

        offsets = (unsigned int *) _alloca(allocSize);
        lines = (unsigned int *) _alloca(allocSize);
        cols = (unsigned int *) _alloca(allocSize);
        endLines = (unsigned int *) _alloca(allocSize);
        endCols = (unsigned int *) _alloca(allocSize);
    }
    // adjust back down...
    count--;

    int i = 0;

    // traverse the sequence points.
    for (current = firstBB; current; current = current->next) {
        DEBUGINFO * debInfo = current->debugInfo;

        if (debInfo) {
            while(debInfo->prev) {
                ASSERT(debInfo->prev->next == debInfo);
                debInfo = debInfo->prev;
            }

            do {
                unsigned offset = (unsigned) debInfo->beginBlock->startOffset + debInfo->beginOffset;
                unsigned endoffset = (unsigned) debInfo->endBlock->startOffset + debInfo->endOffset;

                ASSERT(endoffset >= offset);
                if (offset != endoffset && ((debInfo->begin != debInfo->end) || (debInfo->begin.IsEmpty() && !debInfo->end.IsEmpty()))) {

                    if (offset > 0 && !i) {
                        // we need to insert a record here for any prologue code
                        // we'll associate it w/ the opening {

                        long index = getOpenIndex();
                        if (index >= 0) {
                            TOKENDATA pd = getPosFromIndex(index);
                            if (pd.iLength != 0) {
                                cols[0] = pd.posTokenStart.iChar+1;
                                lines[0] = pd.posTokenStart.iLine;
                                endCols[0] = pd.posTokenStop.iChar+1;
                                endLines[0] = pd.posTokenStop.iLine;
                                offsets[0] = 0;
                                i++;
                                count++;
                            }
                        }
                    }

                    // record (1-based) column numbers
                    cols[i] = debInfo->begin.iChar + 1;
                    endCols[i] = debInfo->end.iChar + 1;
                    // record the line numbers, but wait to add one
                    // until after they have been mapped
                    lines[i] = debInfo->begin.iLine;
                    endLines[i] = debInfo->end.iLine;
                    // record IL offset.
                    offsets[i] = offset;

                    // If the begin is not empty, but the end is, this means
                    // That we don't want to emit any debug info for this code
                    if (debInfo->end.IsEmpty()) {
                        cols[i] = endCols[i] = 0;
                        lines[i] = endLines[i] = NO_DEBUG_LINE;
                    }

                    // Dont actually emit this info if it is exactly the same as the previous one
                    int check = i - 1;
                    if (check >= 0 &&
                        cols[check] == cols[i] && endCols[check] == endCols[i] &&
                        lines[check] == lines[i] && endLines[check] == endLines[i])
                    {
                        --count;
                    }
                    else
                    {
                        // advance to next entry.
                        ++i;
                    }
                } else {
                    // offset==endoffset -> no actual code emitted, so no sequence point.
                    // or begin == end -> no text to associate the code with, so no sequence point
                    --count;
                }
                debInfo = debInfo->next;
            } while(debInfo);
        }
    }

    ASSERT(i == count);

    // And emit the sequence points.
    compiler()->emitter.EmitDebugBlock(method, count, offsets, lines, cols, endLines, endCols);

    // emit debug info for temporaries
    TEMPBUCKET * bucket = temporaries;
    int cTemp = 0;
    while (bucket) {
        for (int i= 0; i < TEMPBUCKETSIZE && bucket->slots[i].type; i += 1) {
            WCHAR nameBuffer[MAX_IDENT_SIZE];
            swprintf(nameBuffer, TEMPORARY_NAME_PREFIX L"%08d$%08d", bucket->slots[i].tempKind, cTemp);
            compiler()->emitter.EmitDebugTemporary(bucket->slots[i].type, nameBuffer, bucket->slots[i].ilSlot);

            cTemp += 1;
        }

        bucket = bucket->next;
    }

    if (localScope && localScope->tree) {
        // Emit information of local variables and their containing scopes.

        emitDebugScopesAndVars(localScope, codeSize);

    }

    compiler()->emitter.EmitDebugMethodInfoStop(method, codeSize);
}

void ILGENREC::emitDebugScopesAndVars(SCOPESYM * scope, unsigned codeSize)
{
    // Walk all locals declared in this scope. Emit scope information only if we're emitting
    // at least one variable. We can't do scopes and variables in a single loop because we need
    // to do an EmitDebugScopeStart (if one is needed) before doing any sub-scopes to get the
    // proper nesting of scopes.
    SCOPESYM * scopeOpened = NULL;
    unsigned beginOffset = 0;
    unsigned endOffset = codeSize;

    SYM * current = scope->firstChild;
    while (current) {
        if (current->kind == SK_LOCVARSYM && (current->asLOCVARSYM()->slot.type || current->asLOCVARSYM()->isConst)) {
            // Open the current scope, if needed. If the current scope is the top-most scope
            // this isn't needed.
            if (!scopeOpened && scope != localScope) {
                // Opening a new scope.
                scopeOpened = scope;
                while (!scopeOpened->debugBlockStart && scopeOpened != localScope)
                    scopeOpened = scopeOpened->parent->asSCOPESYM();

                if (scopeOpened == localScope)
                    scopeOpened = NULL;  // don't reopen top-level scope.
                else {
                    beginOffset = scopeOpened->debugBlockStart->startOffset;
                    endOffset = scopeOpened->debugBlockEnd->startOffset;
                    // This block/scope may have been wiped because it was unreachable
                    // so be careful about emitting an offest past the end of the function
                    if (scopeOpened->debugBlockStart->curLen < scopeOpened->debugOffsetStart)
                        beginOffset += (unsigned)scopeOpened->debugBlockStart->curLen;
                    else
                        beginOffset += scopeOpened->debugOffsetStart;
                    if (scopeOpened->debugBlockEnd->curLen < scopeOpened->debugOffsetEnd)
                        endOffset += (unsigned)scopeOpened->debugBlockEnd->curLen;
                    else
                        endOffset += scopeOpened->debugOffsetEnd;

                    ASSERT(beginOffset <= endOffset && endOffset <= codeSize);
                    if (beginOffset == endOffset)
                        // this is a zero length scope.  No need to emit any nested variables or scopes
                        // just bail here
                        return;

                    compiler()->emitter.EmitDebugScopeStart(beginOffset);
                }
            }

            if (current->asLOCVARSYM()->isConst) {
                // Emit the constant.
                compiler()->emitter.EmitDebugLocalConst(current->asLOCVARSYM());
            }
            else {
                // Emit the variable, along with the IL offsets it is valid for (from first use to end of scope).
                if (current->asLOCVARSYM()->debugBlockFirstUsed) {
                    compiler()->emitter.EmitDebugLocal(current->asLOCVARSYM(), 
                                                       current->asLOCVARSYM()->debugBlockFirstUsed->startOffset + current->asLOCVARSYM()->debugOffsetFirstUsed, 
                                                       endOffset);
                }
                else {
                    // Unknown IL offsets: (0,0) means whole scope.
                    compiler()->emitter.EmitDebugLocal(current->asLOCVARSYM(), 0, 0);
                }
            }
        }
        current = current->nextChild;
    }

    // Walk all sub-scopes and emit them.
    current = scope->firstChild;
    while (current) {
        if (current->kind == SK_SCOPESYM) {
            emitDebugScopesAndVars(current->asSCOPESYM(), codeSize);
        }
        current = current->nextChild;
    }

    // If we opened a scope, close it.
    if (scopeOpened) {
        compiler()->emitter.EmitDebugScopeEnd(endOffset);
    }
}



bool ILGENREC::fitsInBucket(PSWITCHBUCKET bucket, unsigned __int64 keyNext, unsigned newMembers)
{
    if (!bucket) return false;
    
    return (bucket->members + newMembers) * 2 > (keyNext - bucket->firstMember);    
}

unsigned ILGENREC::mergeLastBucket(PSWITCHBUCKET * lastBucket)
{

    unsigned merged = 0;

    PSWITCHBUCKET currentBucket = *lastBucket;

AGAIN:
    PSWITCHBUCKET prevBucket = currentBucket->prevBucket;

    if (fitsInBucket(prevBucket, currentBucket->nextMember, currentBucket->members)) {
        *lastBucket = prevBucket;
        prevBucket->nextMember = currentBucket->nextMember;
        prevBucket->members += currentBucket->members;
        currentBucket = prevBucket;
        merged++;

        goto AGAIN;
    }

    return merged;
}


void ILGENREC::emitSwitchBucket(PSWITCHBUCKET bucket, PSLOT slot, BBLOCK * defBlock)
{
    // If this bucket holds 1 member only we dispense w/ the switch statement...
    if (bucket->members == 1) {
        // Use a simple compare...
        dumpLocal(slot, false);
        genExpr(bucket->labels[0]->key);
        curStack -= 2;
        startNewBB(NULL, CEE_BEQ, bucket->labels[0]->block = createNewBB());

        return;
    } 

    BBLOCK * guardBlock = emitSwitchBucketGuard(bucket->labels[bucket->members-1]->key, slot, false);
	if (guardBlock) {
		dumpLocal(slot, false);
		genExpr(bucket->labels[0]->key);
		curStack -= 2;
        startNewBB(NULL, bucket->labels[0]->key->type->isUnsigned() ? CEE_BLT_UN : CEE_BLT, defBlock);
	}

    unsigned __int64 expectedKey = bucket->labels[0]->key->asCONSTANT()->getI64Value();

    dumpLocal(slot, false);
    // Ok, we now need to normalize the key to 0
    if (expectedKey != 0) {
        genExpr(bucket->labels[0]->key);
        putOpcode(CEE_SUB);
    }
    if (guardBlock) {
        putOpcode(CEE_CONV_I4);
    }
    curStack--;

    // Now, lets construct the target blocks...

    SWITCHDEST * switchDest = (SWITCHDEST*) allocator->Alloc(sizeof(SWITCHDEST) + sizeof(SWITCHDESTGOTO) * (int)(bucket->nextMember - bucket->firstMember));

    unsigned slotNum = 0;
    for (unsigned i = 0; i < bucket->members; i++) {
AGAIN:
        switchDest->blocks[slotNum].jumpIntoTry = false;
        if (expectedKey == (unsigned __int64) bucket->labels[i]->key->asCONSTANT()->getI64Value()) {
            switchDest->blocks[slotNum++].dest = bucket->labels[i]->block = createNewBB();
            expectedKey++;
        } else {
            switchDest->blocks[slotNum++].dest = defBlock;
            expectedKey++;
            goto AGAIN;
        }
    }
    switchDest->count = slotNum;

    ASSERT(expectedKey == bucket->nextMember);
    ASSERT(slotNum == (bucket->nextMember - bucket->firstMember));

    startNewBB(guardBlock, CEE_SWITCH, (BBLOCK*) switchDest);
    
}

BBLOCK * ILGENREC::emitSwitchBucketGuard(EXPR * key, PSLOT slot, bool force)
{
    
    FUNDTYPE ft;

    if (!force && (ft = key->type->underlyingType()->fundType()) != FT_I8 && ft != FT_U8) return NULL;

    dumpLocal(slot, false);
    genExpr(key);
    curStack -= 2;

    BBLOCK * rval;
    startNewBB(NULL, key->type->isUnsigned() ? CEE_BGT_UN : CEE_BGT, rval = createNewBB());

    return rval;   
}

void ILGENREC::emitSwitchBuckets(PSWITCHBUCKET * array, unsigned first, unsigned last, PSLOT slot, BBLOCK * defBlock)
{
    if (first == last) {
        emitSwitchBucket(array[first], slot, defBlock);

        return;
    }
    unsigned mid = (last + first + 1) / 2;
    // This way (0 1 2 3) will produce a mid of 2 while
    // (0 1 2) will produce a mid of 1

    // Now, the first half is first to mid-1
    // and the second half is mid to last.

    // If the first half consists of only one bucket, then we will automatically fall into
    // the second half if we fail that switch.  Otherwise, however, we need to check 
    // ourselves which half we belong to:
    if (first != mid - 1) {

        EXPRSWITCHLABEL * lastLabel = array[mid-1]->labels[array[mid-1]->members - 1];
        EXPR * lastKey = lastLabel->key;

        BBLOCK * secondHalf = emitSwitchBucketGuard(lastKey, slot, true);
    
        emitSwitchBuckets(array, first, mid - 1, slot, defBlock);

        startNewBB(secondHalf, CEE_BR, defBlock);
    
    } else {
        emitSwitchBucket (array[first], slot, defBlock);
    }

    emitSwitchBuckets(array, mid, last, slot, defBlock);   

    startNewBB(NULL, CEE_BR, defBlock);
    
}


void ILGENREC::genStringSwitch(EXPRSWITCH * tree)
{
    ASSERT(tree->arg->type->isPredefType(PT_STRING));

    EXPRSWITCHLABEL ** start = tree->labels;
    EXPRSWITCHLABEL ** end = start + tree->labelCount;
    BBLOCK * defBlock = createNewBB();
    BBLOCK * nullBlock;
    if (tree->nullLabel) {
        tree->nullLabel->block = nullBlock = createNewBB();
    } else {
        nullBlock = defBlock;
    }

    if (tree->arg->kind != EK_CONSTANT) {

        if (start != end) {

            if (tree->flags & EXF_HASHTABLESWITCH) {

                SWITCHDEST * switchDest = (SWITCHDEST*) allocator->Alloc(sizeof(SWITCHDEST) + sizeof(SWITCHDESTGOTO) * tree->labelCount);

                switchDest->count = tree->labelCount;

                genExpr(tree->arg);
                putOpcode(CEE_DUP);
                PSLOT slot = STOREINTEMP(compiler()->symmgr.GetPredefType(PT_OBJECT), TK_SHORTLIVED);
                startNewBB(NULL, CEE_BRFALSE, nullBlock);
                curStack--;
                putOpcode(CEE_VOLATILE);
                putOpcode(CEE_LDSFLD);
                putDWORD(tree->hashtableToken);
                dumpLocal(slot, false);
                putOpcode(CEE_CALL);
                emitMethodToken(info->hTableGet);
                curStack--;
                putOpcode(CEE_DUP);
                dumpLocal(slot, true);
                startNewBB(NULL, CEE_BRFALSE, defBlock);
                curStack--;
                dumpLocal(slot, false);
                freeTemporary(slot);
                putOpcode(CEE_UNBOX);
                emitTypeToken(compiler()->symmgr.GetPredefType(PT_INT));
                putOpcode(CEE_LDIND_I4);
                curStack--;
                int count = 0;
                while (start < end) {
                    if (start[0]->key->asCONSTANT()->isNull()) {
                        switchDest->count --;
                    } else {
                        switchDest->blocks[count].jumpIntoTry = false;
                        ASSERT(!start[0]->block);
                        switchDest->blocks[count++].dest = start[0]->block = createNewBB();
                    }
                    start ++;
                }
                startNewBB(NULL, CEE_SWITCH, (BBLOCK*) switchDest);

            } else {

                // strings are not interned until the actual ldstr, so we need to emit
                // ldstr for every string in the switch...
                {
                    EXPRSWITCHLABEL ** tempStart = start;
                    BBLOCK * afterLeave = createNewBB();                    
                    int oldStack = curStack;
                    while (tempStart < end) {
                        if (!tempStart[0]->key->asCONSTANT()->isNull()) {
                            genString(tempStart[0]->key->asCONSTANT()->getSVal());
                        }
                        tempStart++;
                    }
                    if (curStack != oldStack) {
                        curStack = oldStack;
                        startNewBB(afterLeave, CEE_LEAVE, afterLeave);
                    }
                }

                genExpr(tree->arg);
                putOpcode(CEE_DUP);
                PSLOT slot = STOREINTEMP(compiler()->symmgr.GetPredefType(PT_STRING), TK_SHORTLIVED);
                startNewBB(NULL, CEE_BRFALSE, nullBlock);
                curStack--;

                dumpLocal(slot, false);
                putOpcode(CEE_CALL);
                emitMethodToken(info->hIntern);

                dumpLocal(slot, true);

                while (start < end) {
                    if (!start[0]->key->asCONSTANT()->isNull()) {
                        dumpLocal(slot, false);
                        genString(start[0]->key->asCONSTANT()->getSVal());
                        startNewBB(NULL, CEE_BEQ, start[0]->block = createNewBB());
                        curStack -= 2;
                    }
                    start++;
                }
                freeTemporary(slot);
            }

        } else {
            genSideEffects(tree->arg);
        }

        if (tree->flags & EXF_HASDEFAULT) {
            startNewBB(NULL, CEE_BR, (*end)->block = defBlock);
        } else {
            startNewBB(NULL, CEE_BR, tree->breakLabel->block = defBlock);
        }
    }

       
    closeDebugInfo();

    if (tree->bodies) {
        genStatement(tree->bodies);
    }
}


void ILGENREC::genSwitch(EXPRSWITCH *tree)
{

    if (tree->flags & EXF_HASDEFAULT) {
        tree->labelCount --;
    }

    if (tree->arg->type->fundType() == FT_REF) {
        genStringSwitch(tree);
        return;
    }
   
    PSWITCHBUCKET lastBucket = NULL;
    unsigned bucketCount = 0;

    POSDATA endInfo;
    endInfo.SetEmpty();
    EXPRSWITCHLABEL ** start = tree->labels;
    EXPRSWITCHLABEL ** end = start + tree->labelCount;
    BBLOCK * defBlock = createNewBB();

    if (tree->arg->kind == EK_CONSTANT) {

        end = NULL;

    } else {
    
        while (start < end) {
            // First, see if we fit in the last bucket, or if we need to start a new one...
            unsigned __int64 keyNext = (*start)->key->asCONSTANT()->getI64Value() + 1;
            if (fitsInBucket(lastBucket, keyNext, 1)) {
                lastBucket->nextMember = keyNext;
                lastBucket->members ++;
                bucketCount -= mergeLastBucket(&lastBucket);
            } else {
                // create a new bucket...
                PSWITCHBUCKET newBucket = (PSWITCHBUCKET) _alloca(sizeof(SWITCHBUCKET));
                newBucket->firstMember = keyNext - 1;
                newBucket->nextMember = keyNext;
                newBucket->labels = start;
                newBucket->members = 1;
                newBucket->prevBucket = lastBucket;
                lastBucket = newBucket;
                bucketCount++;
            }
            start++;
        }

        // Ok, now to copy all this into an array so that we can do a binary traversal on it:

        PSWITCHBUCKET * bucketArray = (PSWITCHBUCKET*) _alloca(sizeof(PSWITCHBUCKET) * bucketCount);

        for (int i = (int) bucketCount - 1; i >= 0; i--) {
            ASSERT(lastBucket);
            bucketArray[i] = lastBucket;
            lastBucket = lastBucket->prevBucket;
        }
        ASSERT(!lastBucket);

        if (bucketCount) {
            genExpr(tree->arg);

            // Save the ending line/col so we don't include the case labels
            // when we emit the buckets
            if (curDebugInfo) endInfo = curDebugInfo->end;

            PSLOT temp = STOREINTEMP(tree->arg->type, TK_SHORTLIVED);

            emitSwitchBuckets(bucketArray, 0, bucketCount - 1, temp, defBlock);
            freeTemporary(temp);
        } else {
            genSideEffects(tree->arg);
        }
        
        if (tree->flags & EXF_HASDEFAULT) {
            startNewBB(NULL, CEE_BR, (*end)->block = defBlock);
        } else {
            startNewBB(NULL, CEE_BR, tree->breakLabel->block = defBlock);
        }
    }

    if (curDebugInfo) {
        if (!endInfo.IsEmpty())
            // Restore the end info (overwritting whatever ending pos was set by emitSwitchBuckets)
            curDebugInfo->end = endInfo;
        closeDebugInfo();
    }

    if (tree->bodies) {
        genStatement(tree->bodies);
    }

}


// generate code for a label, this merely involves starting the right bblock.
void ILGENREC::genLabel(EXPRLABEL * tree)
{
    if (tree->block) {
        // if we got a block for this, then just make this the current block...
        startNewBB(tree->block);
    } else {
        startNewBB(NULL);
        tree->block = currentBB;
    }
}

// generate code for a goto...
void ILGENREC::genGoto(EXPRGOTO * tree)
{
    ASSERT(!(tree->flags & EXF_UNREALIZEDGOTO));

    BBLOCK * dest = tree->label->block;

    if (tree->flags & EXF_OPTIMIZABLEGOTO && !compiler()->options.m_fOPTIMIZATIONS) {
        startNewBB(NULL);
        inlineBB.v.mayRemove = true;
    }

    if (!dest) {
        dest = tree->label->block = createNewBB(false);
    }
    if (tree->flags & EXF_GOTOASLEAVE) {
        if (tree->flags & EXF_GOTOBLOCKED) {
            inlineBB.v.gotoBlocked = true;
            blockedLeave++;
        }
        startNewBB(NULL, CEE_LEAVE, dest);
    } else {
        startNewBB(NULL, CEE_BR, dest);
    }
}

// generate code for a gotoif...
void ILGENREC::genGotoIf(EXPRGOTOIF * tree)
{
    // this had to be taken care of by the def-use pass...
    ASSERT(!(tree->flags & EXF_UNREALIZEDGOTO));

    BBLOCK * dest = tree->label->block;
    
    if (!dest) {
        dest = tree->label->block = createNewBB();
    }

    genCondBranch(tree->condition, dest, tree->sense);
}

// generate a conditional (ie, boolean) expression...
// this will leave a value on the stack which conforms to sense, ie:
// condition XOR sense
void ILGENREC::genCondExpr(EXPR * condition, bool sense)
{
    ILCODE ilcode;
    unsigned idx;
    unsigned senseidx;
    int UNoffset;

AGAIN:

    UNoffset = 0;

#if DEBUG
    int prevStack = curStack;
    if (condition->flags & EXF_BINOP && condition->asBIN()->p1->kind == EK_WRAP)
        prevStack--;
#endif

    switch(condition->kind) {
    case EK_LOGAND: 
        // remember that a XOR false is ~a, and so ~(a & B) is done as (~a || ~b)...
        if (!sense) goto LOGOR;
LOGAND: {
            // we generate:
            // gotoif (a XOR !sense) lab0
            // b XOR sense 
            // goto labEnd
            // lab0
            // 0
            // labEnd
            BBLOCK * fallThrough = genCondBranch(condition->asBIN()->p1, createNewBB(), !sense);
            genCondExpr(condition->asBIN()->p2, sense);
            ASSERT(prevStack + 1 == curStack);
            curStack--;
            BBLOCK * labEnd = createNewBB(false);
            startNewBB(fallThrough, CEE_BR, labEnd);
            genIntConstant(0);
            startNewBB(labEnd);
            break;
        }
        
    case EK_LOGOR:
        // as above, ~(a || b) is (~a & ~b)
        if (!sense) goto LOGAND;
LOGOR:  {
            // we generate:
            // gotoif (a XOR sense) lab1
            // b XOR sense
            // goto labEnd
            // lab1
            // 1
            // labEnd
            BBLOCK * ldOne = genCondBranch(condition->asBIN()->p1, createNewBB(), sense);
            genCondExpr(condition->asBIN()->p2, sense);
            ASSERT(prevStack + 1 == curStack);
            curStack--;
            BBLOCK * fallThrough = createNewBB(false);
            startNewBB(ldOne, CEE_BR, fallThrough);
            genIntConstant(1);
            startNewBB(fallThrough);
            break;
        }

    case EK_CONSTANT:
        // make sure that only the bool bits are set:
        ASSERT(condition->asCONSTANT()->getVal().iVal == 0 || condition->asCONSTANT()->getVal().iVal == 1);

        genIntConstant((condition->asCONSTANT()->getVal().iVal == (int)sense) ? 1 : 0);
        break;
    case EK_LT:
    case EK_LE:
    case EK_GT:
    case EK_GE: {
        if (condition->asBIN()->p1->type->fundType() == FT_R4 || condition->asBIN()->p1->type->fundType() == FT_R8) {
            UNoffset = 1;
        }

    case EK_EQ:
    case EK_NE:

        idx = condition->kind - EK_EQ;
        genExpr(condition->asBIN()->p1);
        genExpr(condition->asBIN()->p2);
        senseidx = sense ? 0 : 2;

        ilcode = ILcjInstr[idx][senseidx];
        ILCODE ilcodeReal;
		if (ilcode != CEE_ILLEGAL && UNoffset) {
            ilcodeReal = (ILCODE) (ILcjInstrUN[idx][senseidx+1]);
        } else {
			ilcodeReal = (ILCODE) (ILcjInstr[idx][senseidx+1]);
		}
        if (condition->asBIN()->p1->type->isUnsigned()) {
            ASSERT(condition->asBIN()->p2->type->isUnsigned());
            if (ilcodeReal == CEE_CGT)
                ilcodeReal = CEE_CGT_UN;
            if (ilcodeReal == CEE_CLT)
                ilcodeReal = CEE_CLT_UN;
        }
        putOpcode(ilcodeReal);
        if (ilcode != CEE_ILLEGAL) {
            ASSERT(ilcode == CEE_NEG);
            putOpcode(CEE_LDC_I4_0);
            putOpcode(CEE_CEQ);
        }
        break;
        }
    case EK_LOGNOT:
        sense = !sense;
        condition = condition->asBIN()->p1;
        goto AGAIN;
    default:
        genExpr(condition);
        if (!sense) {
            putOpcode(CEE_LDC_I4_0);
            putOpcode(CEE_CEQ);
        }
    }

}


bool ILGENREC::isSimpleExpr(EXPR * condition, bool * sense)
{
    EXPR * op1 = condition->asBIN()->p1;
    EXPR * op2 = condition->asBIN()->p2;

    bool c1 = op1->kind == EK_CONSTANT;
    bool c2 = op2->kind == EK_CONSTANT;

    if (!c1 && !c2) return false;

    EXPR * constOp;
    EXPR * nonConstOp;

    if (c1) {
        constOp = op1;
        nonConstOp = op2;
    } else {
        constOp = op2;
        nonConstOp = op1;
    }

    FUNDTYPE ft = nonConstOp->type->fundType();
    if (ft == FT_NONE || (ft >= FT_I8 && ft <= FT_R8)) return false;

    bool isBool = nonConstOp->type->isPredefType(PT_BOOL);
    bool isZero = (ft == FT_I8 || ft == FT_U8) ? constOp->asCONSTANT()->getI64Value() == 0 : constOp->asCONSTANT()->getVal().iVal == 0;

    // bool is special, onyl it can be compared to true and false...
    if (!isBool && !isZero) {
        return false;
    }

    // if comparing to zero, flip the sense
    if (isZero) {
        *sense = !(*sense);
    }

    // if comparing != flip the sense
    if (condition->kind == EK_NE) {
        *sense = !(*sense);
    }

    genExpr(nonConstOp);

    return true;


}


// generate a jump to dest if condition XOR sense is true
BBLOCK * ILGENREC::genCondBranch(EXPR * condition, BBLOCK * dest, bool sense)
{

    ASSERT(dest);

    ILCODE ilcode;
    unsigned idx;
    unsigned senseidx;
    int UNoffset;
    bool reverseToUN;

AGAIN:

    UNoffset = 0;
    reverseToUN = false;

    switch(condition->kind) {
    case EK_LOGAND: {
        // see comment in genCondExpr
        if (!sense) goto LOGOR;
LOGAND:
        // we generate:
        // gotoif(a XOR !sense) labFT
        // gotoif(b XOR sense) labDest
        // labFT
        BBLOCK * fallThrough = genCondBranch(condition->asBIN()->p1, createNewBB(), !sense);
        genCondBranch(condition->asBIN()->p2, dest, sense);
        startNewBB(fallThrough);
        return dest;
        }
    case EK_LOGOR:
        if (!sense) goto LOGAND;
LOGOR:
        // we generate:
        // gotoif(a XOR sense) labDest
        // gotoif(b XOR sense) labDest
        genCondBranch(condition->asBIN()->p1, dest, sense);
        condition = condition->asBIN()->p2;
        goto AGAIN;

    case EK_CONSTANT:
        // make sure that only the bool bits are set:
        ASSERT(condition->asCONSTANT()->getVal().iVal == 0 || condition->asCONSTANT()->getVal().iVal == 1);

        if (condition->asCONSTANT()->getVal().iVal == (int)sense) {
            ilcode = CEE_BR;
            break;
        } // otherwise this branch will never be taken, so just fall through...
        return dest;
    case EK_LT:
    case EK_LE:
    case EK_GT:
    case EK_GE:
        if (condition->asBIN()->p1->type->fundType() == FT_R4 || condition->asBIN()->p1->type->fundType() == FT_R8) {
            reverseToUN = true;
            if (!sense) {
                UNoffset = 2;
            }
        } else if (condition->asBIN()->p1->type->isUnsigned()) {
            ASSERT(condition->asBIN()->p2->type->isUnsigned());
            UNoffset = 1;
        }

        goto SKIPSIMPLE;

    case EK_EQ:
    case EK_NE:

        if (isSimpleExpr(condition, &sense)) goto SIMPLEBR;

SKIPSIMPLE:

        idx = condition->kind - EK_EQ;
        genExpr(condition->asBIN()->p1);
        genExpr(condition->asBIN()->p2);
        senseidx = sense ? 4 : 5;

		if (UNoffset) {
			ilcode = ILcjInstrUN[idx][senseidx];
		} else {
			ilcode = ILcjInstr[idx][senseidx];
		}

        break;
    case EK_LOGNOT:
        sense = !sense;
        condition = condition->asBIN()->p1;
        goto AGAIN;

    case EK_IS:
    case EK_AS:
        if (condition->asBIN()->p2->kind == EK_TYPEOF) {
            genExpr(condition->asBIN()->p1);
            putOpcode(CEE_ISINST);
            emitTypeToken(condition->asBIN()->p2->asTYPEOF()->sourceType);
            goto SIMPLEBR;
        }

    default:
        genExpr(condition);
SIMPLEBR:
        ilcode = sense ? CEE_BRTRUE : CEE_BRFALSE;
    }

    inlineBB.v.condIsUN = reverseToUN;
    startNewBB(NULL, ilcode, dest);
    // since we are not emitting the instruction, but rather are saving it in the bblock,
    // we need to manipulate the stack ourselves...
    curStack += ILStackOps[ilcode];
    markStackMax();

    return dest;
}



// generate code for a return statement
void ILGENREC::genReturn(EXPRRETURN * tree)
{
    if (tree->object) {
        genExpr(tree->object);
    }

    if (tree->flags & EXF_RETURNASLEAVE || !compiler()->options.m_fOPTIMIZATIONS) {
        if (!returnLocation) {
            returnLocation = createNewBB();
            returnHandled = false;
        }
        if (retTemp) {
            dumpLocal(retTemp, true);
        } else if (tree->object) {
            // this could only happen if this is unreachable code...
            curStack--;
        }
        if (tree->flags & EXF_RETURNBLOCKED) {
            inlineBB.v.gotoBlocked = true;
            blockedLeave++;
        }
        if (tree->flags & EXF_RETURNASLEAVE) {
            startNewBB(NULL, CEE_LEAVE, returnLocation);
        } else {
            startNewBB(NULL, CEE_BR, returnLocation);
        }
    } else {

        handleReturn();
        startNewBB(NULL, CEE_RET);
    }
}


// generate a string constant
void ILGENREC::genString(CONSTVAL string)
{
    putOpcode(CEE_LDSTR);
    ULONG rva = compiler()->emitter.GetStringRef(string.strVal);
    putDWORD(rva);
}


void ILGENREC::genSwap(EXPRBINOP * tree, PVALUSED valUsed)
{

    if (!valUsed) {
        genSideEffects(tree);
        return;
    }

    genExpr(tree->p1);
    PSLOT slot = STOREINTEMP(tree->p2->type, TK_SHORTLIVED);
    genExpr(tree->p2);
    dumpLocal(slot, false);
    freeTemporary(slot);
}

// generate "is" or "as", depending on isAs.
void ILGENREC::genIs(EXPRBINOP * tree, PVALUSED valUsed, bool isAs)
{

    if (!valUsed) {
        genSideEffects(tree);
        return;
    }
    
    EXPR * e1 = tree->p1;
    EXPR * e2 = tree->p2;

    if (e2->kind == EK_TYPEOF) {

        genExpr(e1);
        putOpcode(CEE_ISINST);
        emitTypeToken(e2->asTYPEOF()->sourceType);
        if (!isAs) {
            putOpcode(CEE_LDNULL);
            putOpcode(CEE_CGT_UN);
        }

        return;
    }


    ASSERT(!isAs);

    BBLOCK * fallThrough, * push0;

    genExpr(e1);
    e2->asCALL()->args->asWRAP()->slot = STOREINTEMP(e1->type, TK_SHORTLIVED);
    dumpLocal(e2->asCALL()->args->asWRAP()->slot, false);

    startNewBB(NULL, CEE_BRFALSE, push0 = createNewBB());
    curStack--;

    genExpr(e2);

    curStack--;
    startNewBB(push0, CEE_BR, fallThrough = createNewBB());

    genIntConstant(0);

    startNewBB(fallThrough);


}

void ILGENREC::genPtrAddr(EXPR * op, PVALUSED valUsed)
{
    // strings are special...
    if (op->type->kind == SK_PINNEDSYM && op->type == compiler()->symmgr.GetPinnedType(compiler()->symmgr.GetPredefType(PT_STRING))) {
        genExpr(op);
        putOpcode(CEE_CONV_I);
        putOpcode(CEE_CALL);
        emitMethodToken(info->hStringOffset);
        curStack++;
        markStackMax();
        putOpcode(CEE_ADD);
    } else {
        genMemoryAddress(op, NULL, true);
    }

    if (!valUsed)  {
        putOpcode(CEE_POP);
    }


}


void ILGENREC::genTypeRefAny(EXPRBINOP * tree, PVALUSED valUsed)
{
    if (!valUsed) {
        genSideEffects(tree->p1);
        return;
    }

    genExpr(tree->p1);
    putOpcode(CEE_REFANYTYPE);
    putOpcode(CEE_CALL);
    emitMethodToken(tree->p2->asTYPEOF()->method);
}

void ILGENREC::genMakeRefAny(EXPRBINOP * tree, PVALUSED valUsed)
{
    if (!valUsed) {
        genSideEffects(tree->p1);
        return;
    }
    
    genMemoryAddress(tree->p1, NULL);
    putOpcode(CEE_MKREFANY);
    emitTypeToken(tree->p1->type);
}

// generate a generic expression
void ILGENREC::genExpr(EXPR * tree, PVALUSED valUsed)
{
AGAIN:

    if (!tree) {
        return;
    }

    switch (tree->kind) {
    case EK_CONCAT:
        if (tree->flags & EXF_UNREALIZEDCONCAT) {
            // this occurs in unreachable code, so just bump the stack pointer...
            if (valUsed) { 
                curStack++;
                markStackMax();
            }
            if (tree->flags & EXF_VALONSTACK) curStack--;
            return;
        }
        tree = tree->asCONCAT()->list;
        goto AGAIN;

    case EK_WRAP:

        if (tree->asWRAP()->slot) {
            dumpLocal(tree->asWRAP()->slot, false);
            if (!tree->asWRAP()->doNotFree) {
                freeTemporary(tree->asWRAP()->slot);
            }
        } else if (tree->asWRAP()->expr && tree->asWRAP()->expr->kind == EK_WRAP) {
            if (tree->asWRAP()->expr->asWRAP()->slot && tree->asWRAP()->expr->asWRAP()->doNotFree) {
                freeTemporary(tree->asWRAP()->expr->asWRAP()->slot);
            }
        }
        return;
    case EK_USERLOGOP: {
        genExpr(tree->asUSERLOGOP()->opX);
        putOpcode(CEE_DUP);
        genExpr(tree->asUSERLOGOP()->callTF);
        BBLOCK * target = createNewBB();
        startNewBB(NULL, CEE_BRTRUE, target);
        curStack--;
        genExpr(tree->asUSERLOGOP()->callOp);
        startNewBB(target);
        break;
        }
    case EK_SAVE:
        ASSERT(valUsed);
        genExpr(tree->asBIN()->p1);
        putOpcode(CEE_DUP);
        tree->asBIN()->p2->asWRAP()->slot = STOREINTEMP(tree->asBIN()->p1->type, tree->asBIN()->p2->asWRAP()->tempKind);
        return;
    case EK_LOCALLOC:
        genExpr(tree->asBIN()->p1);
        putOpcode(CEE_LOCALLOC);
        break;
    case EK_QMARK:
        genQMark(tree->asBIN(), valUsed);
        return;
    case EK_SWAP:
        genSwap(tree->asBIN(), valUsed);
        return;
    case EK_IS:
        genIs(tree->asBIN(), valUsed, false);
        return;
    case EK_AS:
        genIs(tree->asBIN(), valUsed, true);
        return;
    case EK_CAST:
        genCast(tree->asCAST(), valUsed);
        return;
    case EK_MAKERA:
        genMakeRefAny(tree->asBIN(), valUsed);
        return;
    case EK_TYPERA:
        genTypeRefAny(tree->asBIN(), valUsed);
        return;
    case EK_FUNCPTR: {
        ILCODE il = (tree->asFUNCPTR()->func->isVirtual && !tree->asFUNCPTR()->func->parent->asAGGSYM()->isSealed && tree->asFUNCPTR()->object->type->fundType() == FT_REF && !(tree->flags & EXF_BASECALL)) ? CEE_LDVIRTFTN : CEE_LDFTN;
        if (il == CEE_LDVIRTFTN) {
            putOpcode (CEE_DUP);
        }
        putOpcode(il);
#if USAGEHACK
        tree->asFUNCPTR()->func->isUsed = true;
#endif
        if (tree->flags & EXF_BASECALL) {
            emitMethodToken(getBaseMethod(tree->asFUNCPTR()->func)->asFMETHSYM());
        } else {
            emitMethodToken(tree->asFUNCPTR()->func);
        }
        break;
                     }
    case EK_TYPEOF:
        if (valUsed) {
            putOpcode(CEE_LDTOKEN);
            emitTypeToken(tree->asTYPEOF()->sourceType);
            putOpcode(CEE_CALL);
            emitMethodToken(tree->asTYPEOF()->method);
        }
        return;
    case EK_SIZEOF:
        if (valUsed) {
            genSizeOf(tree->asSIZEOF()->sourceType);
        }
        return;
    case EK_ARRINIT:
        genArrayInit(tree->asARRINIT(), valUsed);
        return;
    case EK_CONSTANT:
        if (!valUsed) return;
        switch(tree->type->fundType ()) {
        case FT_I1:
        case FT_U1:
        case FT_I2:
        case FT_U2:
        case FT_I4:
        case FT_U4:
            genIntConstant(tree->asCONSTANT()->getVal().iVal); // OK for U4, since IL treats them the same.
            break;

        case FT_R4:
            genFloatConstant((float) * tree->asCONSTANT()->getVal().doubleVal);
            break;
        case FT_R8:
            genDoubleConstant(tree->asCONSTANT()->getVal().doubleVal);
            break;
        case FT_I8:
        case FT_U8:
            genLongConstant(tree->asCONSTANT()->getVal().longVal);
            break;
        case FT_REF:
            if (tree->asCONSTANT()->getSVal().strVal) {
                // this must be a string...
                genString(tree->asCONSTANT()->getSVal());
            } else {
                putOpcode(CEE_LDNULL);
            }
            break;

        case FT_STRUCT:
            ASSERT(tree->type->isPredefType(PT_DECIMAL));
            genDecimalConstant(tree->asCONSTANT()->getVal().decVal);
            break;

        default:
            ASSERT(!"bad constant type");
        }
        break;
    case EK_CALL:
        genCall(tree->asCALL(), valUsed);
        return;
    case EK_NOOP:
        return;
    case EK_MULTI:
        genMultiOp(tree->asMULTI(), valUsed);
        return;
    case EK_LIST:
        // this kind of loop takes less code if we know that the elements
        // are also expressions
        genExpr(tree->asBIN()->p1, valUsed);
        tree = tree->asBIN()->p2;
        goto AGAIN;
    case EK_NEWARRAY:
        genNewArray(tree->asBIN());
        break;
    case EK_SEQUENCE:
        genSideEffects(tree->asBIN()->p1);
        tree = tree->asBIN()->p2;
        goto AGAIN;
    case EK_ARGS:
        putOpcode(CEE_ARGLIST);
        break;
    case EK_LOCAL:
        if (!tree->asLOCAL()->local->slot.isRefParam) {
            dumpLocal(&(tree->asLOCAL()->local->slot), false);
            break;
        }
    case EK_INDIR:
    case EK_VALUERA:
    case EK_PROP:
    case EK_ARRINDEX:
    case EK_FIELD: {
        ADDRESS addr;
        genAddress(tree, &addr);
        genFromAddress(&addr, false, true);
        break;
                   }

    case EK_ZEROINIT:
        genZeroInit(tree->asZEROINIT(), valUsed);
        return;
    case EK_ADDR:
        genPtrAddr(tree->asBIN()->p1, valUsed);
        return;
    case EK_ASSG: {
        ADDRESS addr;
        EXPR * op1 = tree->asBIN()->p1;
        EXPR * op2 = tree->asBIN()->p2;

        if (isExprOptimizedAway(op1)) {
            if (valUsed) {
                genExpr(op2);
            } else {
                genSideEffects(op2);
            }
            return;
        }
        genAddress(op1, &addr);
        genExpr(op2);
        PSLOT slot = NULL;
        if (valUsed) {
            putOpcode(CEE_DUP);
            if (addr.type == ADDR_STACK) {
                slot = allocTemporary(op2->type, TK_SHORTLIVED);
                dumpLocal(slot, true);
            }
        }
        genFromAddress(&addr, true, true);
        if (slot) {
            dumpLocal(slot, false);
            freeTemporary(slot);
        }
        return;
    }
    default:
        if (tree->flags & EXF_BINOP) {
            genBinopExpr(tree->asBIN(), valUsed);
        } else {
            ASSERT(!"bad expr type");
        }
    }


    if (!valUsed) {
        putOpcode(CEE_POP);
    }
    
}

void ILGENREC::genZero(BASENODE * tree, TYPESYM * type)
{
    ASSERT(type->fundType() != FT_STRUCT || type->asAGGSYM()->iPredef == PT_INTPTR);

    if (type->fundType() == FT_REF) {
        putOpcode(CEE_LDNULL);
    } else if (type->fundType() == FT_PTR) {
        genIntConstant(0);
        putOpcode(CEE_CONV_U);
    } else if (type->fundType() == FT_STRUCT) {
        ASSERT(type->asAGGSYM()->iPredef == PT_INTPTR);
        genIntConstant(0);
        putOpcode(CEE_CONV_I);
    } else {
        type = type->underlyingType();
        ASSERT(type->asAGGSYM()->isPredefined);
        EXPRCONSTANT expr;
        expr.kind = EK_CONSTANT;
        expr.getVal().iVal = compiler()->symmgr.GetPredefZero((PREDEFTYPE)(type->asAGGSYM()->iPredef)).iVal;
        expr.type = type;
        expr.tree = tree;
        genExpr(&expr);
    }
}

void ILGENREC::genZeroInit(EXPRZEROINIT * tree, PVALUSED valUsed)
{
    EXPR * op = tree->p1;
    
    if (op && isExprOptimizedAway(op)) {
        op = NULL;
    }

    if (!op && !valUsed) return;


    if (tree->type->fundType() == FT_STRUCT) {
        PSLOT slot = NULL;
        if (op) {
            genMemoryAddress(op, &slot);
            ASSERT(!slot);
            if (valUsed) {
                putOpcode(CEE_DUP);
            }
        } else {
            ASSERT(valUsed);
            slot = allocTemporary(tree->type, TK_SHORTLIVED);
            genSlotAddress(slot);
        }
        putOpcode(CEE_INITOBJ);
        emitTypeToken(tree->type);
        if (valUsed) {
            if (op) {
                putOpcode(CEE_LDOBJ);
                emitTypeToken(tree->type);
            } else {
                dumpLocal(slot, false);
                freeTemporary(slot);
            }

        }
        return;
    }

    ADDRESS addr;

    if (op) {
        genAddress(op, &addr);
        genZero(tree->tree, tree->type);
        genFromAddress(&addr, true, true);
        if (valUsed) {
            genZero(tree->tree, tree->type);
        }
    } else {
        ASSERT(valUsed);
        genZero(tree->tree, tree->type);
    }

}


void ILGENREC::writeArrayValues1(BYTE * buffer, EXPR * tree)
{
    EXPRLOOP(tree, elem)
        if (elem->kind == EK_CONSTANT) {
            EXPRCONSTANT * ec = elem->asCONSTANT();
            *buffer = (BYTE) (ec->getVal().uiVal & 0xff);
        } else {
            *buffer = 0;
        }
        buffer += 1;
    ENDLOOP;

}

void ILGENREC::writeArrayValues2(BYTE * buffer, EXPR * tree)
{
    EXPRLOOP(tree, elem)
        if (elem->kind == EK_CONSTANT) {
            EXPRCONSTANT * ec = elem->asCONSTANT();
            *((WORD*)buffer) = VAL16((WORD) (ec->getVal().uiVal & 0xffff));
        } else {
            *((WORD*)buffer) = (WORD) 0;
        }
        buffer += 2;
    ENDLOOP;

}

void ILGENREC::writeArrayValues4(BYTE * buffer, EXPR * tree)
{
    EXPRLOOP(tree, elem)
        if (elem->kind == EK_CONSTANT) {
            EXPRCONSTANT * ec = elem->asCONSTANT();
            *((DWORD*)buffer) = VAL32((DWORD) (ec->getVal().uiVal));
        } else {
            *((DWORD*)buffer) = (DWORD) 0;
        }
        buffer += 4;
    ENDLOOP;

}

void ILGENREC::writeArrayValues8(BYTE * buffer, EXPR * tree)
{
    EXPRLOOP(tree, elem)
        if (elem->kind == EK_CONSTANT) {
            EXPRCONSTANT * ec = elem->asCONSTANT();
            *((unsigned __int64 *)buffer) = VAL64((unsigned __int64) *(ec->getVal().ulongVal));
        } else {
            *((unsigned __int64 *)buffer) = (unsigned __int64) 0;
        }
        buffer += 8;
    ENDLOOP;
}

void ILGENREC::writeArrayValuesD(BYTE * buffer, EXPR * tree)
{
    ASSERT(sizeof(double) == 8);
    EXPRLOOP(tree, elem)
        if (elem->kind == EK_CONSTANT) {
            __int64 tmp = VAL64(*elem->asCONSTANT()->getVal().ulongVal);
           *(double*)buffer = (double &)tmp;
        } else {
            *((double*)buffer) = (double) 0.0;
        }
        buffer += 8;
    ENDLOOP;
}

void ILGENREC::writeArrayValuesF(BYTE * buffer, EXPR * tree)
{
    ASSERT(sizeof(float) == 4);
    EXPRLOOP(tree, elem)
        if (elem->kind == EK_CONSTANT) {
            EXPRCONSTANT * ec = elem->asCONSTANT();
            float tmpfloat = (float)*ec->getVal().doubleVal;
            __int32 tmp = VAL32(*reinterpret_cast<const __int32 *>(&tmpfloat));
            *((float*)buffer) = (float &)tmp;
        } else {
            *((float*)buffer) = (float) 0.0;
        }
        buffer += 4;
    ENDLOOP;
}

void ILGENREC::genArrayInitConstant(EXPRARRINIT * tree, TYPESYM * elemType, PVALUSED valUsed)
{
    int rank = tree->type->asARRAYSYM()->rank;

    int size, initSize = size = compiler()->symmgr.GetAttrArgSize((PREDEFTYPE)elemType->asAGGSYM()->iPredef);

    ASSERT(initSize > 0);

    for (int i = 0; i < rank; i++) {
        size = size * tree->dimSizes[i];
    }

    BYTE * buffer;
    mdToken token = compiler()->emitter.GetGlobalFieldDef(method, ++globalFieldCount, (unsigned) size, &buffer);
    
    switch (initSize) {
    case 1:
        writeArrayValues1(buffer, tree->args);
        break;
    case 2:
        writeArrayValues2(buffer, tree->args);
        break;
    case 4:
        if (elemType->asAGGSYM()->fundType() == FT_R4) {
            writeArrayValuesF(buffer, tree->args);
        } else {
            writeArrayValues4(buffer, tree->args);
        }
        break;
    case 8:
        if (elemType->asAGGSYM()->fundType() == FT_R8) {
            writeArrayValuesD(buffer, tree->args);
        } else {
            writeArrayValues8(buffer, tree->args);
        }
        break;
    default:
        ASSERT(0);
    }

    if (valUsed || !(tree->flags & EXF_ARRAYALLCONST)) {
        putOpcode(CEE_DUP);
    }

    putOpcode(CEE_LDTOKEN);
    putDWORD(token);
    putOpcode(CEE_CALL);
    emitMethodToken(info->hInitArray);
    curStack -= 2;

}


void ILGENREC::genArrayInit(EXPRARRINIT *tree, PVALUSED valUsed)
{

    int rank = tree->type->asARRAYSYM()->rank;
    TYPESYM * elemType = tree->type->asARRAYSYM()->elementType();

    PSLOT wrappedValue;
    if (tree->flags & EXF_VALONSTACK) {
        wrappedValue = STOREINTEMP(elemType, TK_SHORTLIVED);
		// This only occurs for string/object concatination.
        ASSERT((elemType->isPredefType(PT_STRING)) ||
			   (elemType->isPredefType(PT_OBJECT)));
    } else {
        wrappedValue = NULL;
    }

    for (int i = 0; i < rank; i++) {
        genIntConstant(tree->dimSizes[i]);
    }
    if (rank == 1) {
        putOpcode(CEE_NEWARR);
        emitTypeToken(elemType);
    } else {
        genArrayCall(tree->type->asARRAYSYM(), rank, ARRAYMETH_CTOR);
    }

    if (tree->args && (tree->flags & (EXF_ARRAYCONST | EXF_ARRAYALLCONST))) {
        genArrayInitConstant(tree, elemType, valUsed);
        if (tree->flags & EXF_ARRAYALLCONST) {
            return;
        }
    }
    
    PSLOT slot = allocTemporary(tree->type, TK_SHORTLIVED);
    dumpLocal(slot, true);

    if (tree->args) {

        ILCODE il = ILarrayStore[elemType->fundType()];

        int * rows = (int *) _alloca(sizeof(int) * rank);
        memset(rows, 0, sizeof(int) * rank);
    
        EXPRLOOP(tree->args, elem)
            if (!elem->isDefValue() && (elem->kind != EK_CONSTANT || !(tree->flags & EXF_ARRAYCONST))) {

                dumpLocal(slot, false);
                for (int i = 0; i < rank; i ++) {
                    genIntConstant(rows[i]);
                }
                if (elemType->fundType() == FT_STRUCT) {
                    il = CEE_LDELEMA;
                    if (rank == 1) {
                        putOpcode(il);
                        emitTypeToken(elemType);
                    } else {
                        genArrayCall(tree->type->asARRAYSYM(), rank, ARRAYMETH_LOADADDR);
                    }
                    genExpr(elem);
                    putOpcode(CEE_STOBJ);
                    emitTypeToken(elemType);
                } else {
                    if (wrappedValue) {
                        dumpLocal(wrappedValue, false);
                        freeTemporary(wrappedValue);
                        wrappedValue = NULL;
                    } else {
                        genExpr(elem);
                    }
                    if ((rank == 1) && (il != CEE_ILLEGAL)) {
                        putOpcode(il);
                    } else {
                        genArrayCall(tree->type->asARRAYSYM(), rank, ARRAYMETH_STORE);
                    }
                }
            }
            int row = rank - 1;
            while(true) {
                rows[row]++;
                if (rows[row] == tree->dimSizes[row]) {
                    rows[row] = 0;
                    if (row == 0) {
                        ASSERT(_nd == NULL);
                        goto DONE;
                    }
                    row--;
                } else {
                    break;
                }
            }        
        ENDLOOP;
    }    
DONE:

    if (valUsed) dumpLocal(slot, false);

    freeTemporary(slot);

}


void ILGENREC::genCast(EXPRCAST * tree, PVALUSED valUsed)
{

    int key = tree->flags & (EXF_BOX | EXF_UNBOX | EXF_REFCHECK | EXF_CHECKOVERFLOW | EXF_INDEXEXPR);

    if (!(key & ~(EXF_BOX | EXF_INDEXEXPR)) && !valUsed) {
        genSideEffects(tree->p1);
        return;
    }

    key &= ~EXF_CHECKOVERFLOW;

    ILCODE il;
    PSLOT slot = NULL;
    FUNDTYPE ft;

    TYPESYM * fromType = tree->p1->type->underlyingType();
    TYPESYM * toType = tree->type->underlyingType();

    if (key == EXF_BOX) {
        genExpr(tree->p1);
        putOpcode(CEE_BOX);
        emitTypeToken(tree->p1->type);
    } else if (key == EXF_INDEXEXPR) {
        genExpr(tree->p1);
        if (toType->fundType() == FT_U4) {
            putOpcode(CEE_CONV_U);
        } else if (toType->fundType() == FT_I8) {
            putOpcode(CEE_CONV_OVF_I);
        } else {
            ASSERT(toType->fundType() == FT_U8);
            putOpcode(CEE_CONV_OVF_I_UN);
        }
    } else {
        
        // This is null being cast to a pointer, change this into a zero instead...
        if (toType->kind == SK_PTRSYM && fromType->kind != SK_PTRSYM && tree->p1->kind == EK_CONSTANT && tree->p1->type->fundType() == FT_REF) {
            genIntConstant(0);
        } else {
            genExpr(tree->p1);
        }

        switch(key) {
        case EXF_UNBOX:
            putOpcode(CEE_UNBOX);
            emitTypeToken(tree->type);
            putOpcode(ILstackLoad[ft = tree->type->fundType()]);
            if (ft == FT_STRUCT) {
                emitTypeToken(toType);
            }
            break;
        case EXF_REFCHECK:
            putOpcode(CEE_CASTCLASS);
            emitTypeToken(toType);
            break;
        default:
            if (toType->kind == SK_PTRSYM && fromType->kind != SK_PTRSYM) {
                putOpcode(CEE_CONV_U);
            } else if (fromType->kind == SK_PTRSYM && toType->kind != SK_PTRSYM) {
                PREDEFTYPE pt;
                if (toType->asAGGSYM()->iPredef == PT_LONG) {
                    pt = PT_ULONG;
                } else {
                    pt = (PREDEFTYPE) toType->asAGGSYM()->iPredef;
                }
                if (tree->flags & EXF_CHECKOVERFLOW) {
                    // float is used here since it provides explicit conversions to all the numercal types
                    // [ie, all entries are CONV_something, rather than nops.]
                    il = simpleTypeConversionsOvf[PT_FLOAT][pt];
                } else {
                    il = simpleTypeConversions[PT_FLOAT][pt];
                }
                ASSERT( il != CEE_ILLEGAL);
                if (il != cee_next) {
                    putOpcode(il);
                }
            } else if (fromType->isSimpleType()) {
                if (tree->flags & EXF_CHECKOVERFLOW) {
                    il = simpleTypeConversionsOvf[fromType->asAGGSYM()->iPredef][toType->asAGGSYM()->iPredef];
                } else {
                    il = simpleTypeConversions[fromType->asAGGSYM()->iPredef][toType->asAGGSYM()->iPredef];
                }
                ASSERT( il != CEE_ILLEGAL);
                if (il != cee_next) {
                    putOpcode(il);
                    if (il == CEE_CONV_R_UN) {
                        putOpcode(simpleTypeConversionsEx[fromType->asAGGSYM()->iPredef][toType->asAGGSYM()->iPredef]);
                    }
                }
            }
            break;
        }
    }

    if (!valUsed) putOpcode(CEE_POP);
    if (slot) freeTemporary(slot);
}


void ILGENREC::genQMark(EXPRBINOP * tree, PVALUSED valUsed)
{
    if (!valUsed) {
        genSideEffects(tree);
        return;
    }

    BBLOCK * trueBranch = genCondBranch(tree->p1, createNewBB(), true);
    tree = tree->p2->asBINOP();

    BBLOCK * fallThrough = createNewBB();

    genExpr(tree->p2);
    startNewBB(trueBranch, CEE_BR, fallThrough);
    genExpr(tree->p1);
    curStack--;
    startNewBB(fallThrough);

}

// generate code to create an array...
void ILGENREC::genNewArray(EXPRBINOP * tree)
{
    int rank = tree->type->asARRAYSYM()->rank;
    TYPESYM * elemType = tree->type->asARRAYSYM()->elementType();

    int oldStack = curStack;

    genExpr(tree->p1);
    if (rank == 1) {
        putOpcode(CEE_NEWARR);
        emitTypeToken(elemType);

        return;
    }

    ASSERT(curStack > oldStack);
    genArrayCall(tree->type->asARRAYSYM(), curStack - oldStack,ARRAYMETH_CTOR);
}



void ILGENREC::genMultiOp(EXPRMULTI * tree, PVALUSED valUsed)
{
    ADDRESS addr(ADDR_DUP);
    int needOldValue;
    PSLOT slot = NULL;

    genAddress(tree->left, &addr);

    genFromAddress(&addr, false);
    if (!(needOldValue = (tree->flags & EXF_ISPOSTOP))) {
        genExpr(tree->op);
    }
    if (valUsed) {
        putOpcode(CEE_DUP);
        if (addr.type != ADDR_NONE) {
            slot = allocTemporary(tree->left->type, TK_SHORTLIVED);
            dumpLocal(slot, true);
        }
    }
    if (needOldValue) {
        genExpr(tree->op);
    }
    genFromAddress(&addr, true, true);
    if (slot) {
        dumpLocal(slot, false);
        freeTemporary(slot);
    }
}

// generate a standard binary operation...
void ILGENREC::genBinopExpr(EXPRBINOP * tree, PVALUSED valUsed)
{

    if (tree->flags & EXF_COMPARE) {
        genCondExpr(tree, true);
        return;
    }

    genExpr(tree->p1, VUYES);
    genExpr(tree->p2, VUYES);

    ILCODE ilcode;

    switch (tree->kind) {
    case EK_ADD:
    case EK_SUB:
    case EK_MUL:
    case EK_DIV:
    case EK_MOD:
    case EK_BITAND: 
    case EK_BITOR:
    case EK_BITXOR:
    case EK_LSHIFT:
    case EK_RSHIFT:
    case EK_NEG:
    case EK_BITNOT:
    case EK_ARRLEN:
        if (tree->flags & EXF_CHECKOVERFLOW) {
            if (tree->p1->type->isUnsigned()) {
                ilcode = ILarithInstrUNOvf[tree->kind - EK_ADD];
            } else {
                ilcode = ILarithInstrOvf[tree->kind - EK_ADD];
            } 
        } else {
            if (tree->p1->type->isUnsigned()) {
                ilcode = ILarithInstrUN[tree->kind - EK_ADD];
            } else {
                ilcode = ILarithInstr[tree->kind - EK_ADD];
            }
        }
        break;
    case EK_UPLUS:
        // This is a non-op (we just need to emit the numeric promotion)
        return;
    default:
        ASSERT(!"bad binop expr"); ilcode = CEE_ILLEGAL;
    }

    putOpcode(ilcode);

    if (ilcode == CEE_LDLEN) {
        putOpcode(CEE_CONV_I4);
    }
}

// generate an integer constant by using the smallest possible ilcode
void ILGENREC::genIntConstant(int val)
{
    ASSERT(CEE_LDC_I4_M1 + 9 == CEE_LDC_I4_8);
    if (val >= -1 && val <= 8) {
        putOpcode((ILCODE) (CEE_LDC_I4_0 + val));
        return;
    }

    if (val == (char) (0xFF & val)) {
        putOpcode(CEE_LDC_I4_S);
        putCHAR((char)val);
        return;
    }

    putOpcode(CEE_LDC_I4);
    putDWORD((DWORD)val);
}


void ILGENREC::genFloatConstant(float val)
{
    putOpcode(CEE_LDC_R4);
    putDWORD(*(DWORD*)(&val));
}

void ILGENREC::genDoubleConstant(double * val)
{
    putOpcode(CEE_LDC_R8);
    putQWORD((__int64 *)val);
}

void ILGENREC::genDecimalConstant(DECIMAL * val)
{
    METHSYM *meth = compiler()->symmgr.LookupGlobalSym(compiler()->namemgr->GetPredefName(PN_CTOR), compiler()->symmgr.GetPredefType(PT_DECIMAL), MASK_ALL)->asMETHSYM();
    AGGSYM *int32Sym = compiler()->symmgr.GetPredefType(PT_INT);
    int countArgs = 0;
    
    // check if we can call a constructor without an array
    if (!DECIMAL_SCALE(*val) && DECIMAL_HI32(*val) && (!(DECIMAL_MID32(*val) & 0x80000000) || (DECIMAL_SIGN(*val) && DECIMAL_MID32(*val) == 0x80000000 && DECIMAL_LO32(*val) == 0))) {
        if (!DECIMAL_MID32(*val) && (!(DECIMAL_LO32(*val) & 0x80000000) || (DECIMAL_SIGN(*val) && DECIMAL_LO32(*val) == 0x80000000))) {
            genIntConstant(DECIMAL_SIGN(*val) ? - (int) DECIMAL_LO32(*val) : DECIMAL_LO32(*val));

            while (meth->cParams != 1 || meth->params[0] != int32Sym) {
                meth = meth->nextSameName->asMETHSYM();
            }
        } else {
            __int64 val64;
            val64 = ((ULONGLONG)DECIMAL_MID32(*val) << 32) | DECIMAL_LO32(*val);
            if (DECIMAL_SIGN(*val) & DECIMAL_NEG)
            {
                val64 = -val64;
            }
            genLongConstant(&val64);

            AGGSYM *int64Sym = compiler()->symmgr.GetPredefType(PT_LONG);
            while (meth->cParams != 1 || meth->params[0] != int64Sym) {
                meth = meth->nextSameName->asMETHSYM();
            }
        }
        countArgs = 1;
    } else {

        // new Decimal(lo, mid, hi, sign, scale)
        genIntConstant(DECIMAL_LO32(*val));
        genIntConstant(DECIMAL_MID32(*val));
        genIntConstant(DECIMAL_HI32(*val));
        genIntConstant(DECIMAL_SIGN(*val));
        genIntConstant(DECIMAL_SCALE(*val));

        AGGSYM *bool32Sym = compiler()->symmgr.GetPredefType(PT_BOOL);
        AGGSYM *byte32Sym = compiler()->symmgr.GetPredefType(PT_BYTE);
        while (meth->cParams != 5 || meth->params[0] != int32Sym || meth->params[1] != int32Sym ||
                meth->params[2] != int32Sym || meth->params[3] != bool32Sym || meth->params[4] != byte32Sym) {
            meth = meth->nextSameName->asMETHSYM();
            ASSERT(meth);
        }
        countArgs = 5;
    }

    putOpcode(CEE_NEWOBJ);
    emitMethodToken(meth);

    curStack -= countArgs;
}

// generate a long, but try to do it as an int if it fits...
void ILGENREC::genLongConstant(__int64 * val)
{
    if ((__int32)((*val) & 0xFFFFFFFF) == *val) {
        genIntConstant((__int32)*val);
        putOpcode(CEE_CONV_I8);
    } else if ((unsigned __int32)((*val) & 0xFFFFFFFF) == *(unsigned __int64*)val) {
        genIntConstant((__int32)*val);
        putOpcode(CEE_CONV_U8);
    } else {
        putOpcode(CEE_LDC_I8);
        putQWORD(val);
    }
}

unsigned ILGENREC::getArgCount(EXPR * args)
{
    unsigned rval = 0;
    EXPRLOOP(args, arg)
        rval++;
    ENDLOOP;
    
    return rval;
}

void ILGENREC::storeObjPointerAndArgs(EXPR * obj, EXPR * args, ADDRESS * addr)
{
	bool isArray = addr->expr->kind == EK_ARRINDEX;

    int type = addr->type;

    addr->stackCount = getArgCount(args);

    int stack = type & ADDR_STACK;
    int stackstore = type & ADDR_STACKSTORE;
    int dup = type & ADDR_DUP;
    int store = type & ADDR_STORE;
    int constant = type & ADDR_CONSTANT;



    if (obj) {
        if (!stack) {
            // If DUP, we duplicate always if there are no following args.  But if there are
            // following args, only duplicate if not constant.
            // If STACKSTORE, duplicate only if not constant
            if ((dup && (!addr->stackCount || !constant)) || (stackstore && !constant)) {
                putOpcode(CEE_DUP); // the obj is on the stack twice
            }

            // We never store if its a constant.  Otherwise, store if dup and following args,
            // or if something other than a dup...
            if (!constant && ((dup && addr->stackCount) || !dup)) {
                ASSERT(!addr->baseSlot);
                addr->baseSlot = allocTemporaryRef(obj->type, TK_SHORTLIVED);
                dumpLocal(addr->baseSlot, true);
            }
        }
    }
    
    if (!args) {
        ASSERT(addr->stackCount == 0);
        return;
    }
    
    unsigned count = 0;
    int singleArg;
    if (args->kind == EK_LIST) {
        addr->argSlots = (PSLOT*) allocator->Alloc(sizeof(PSLOT) * addr->stackCount);
        singleArg = 0;
    } else {
        singleArg = 1;
        addr->stackCount = (unsigned) -1;
    }

    EXPRLOOP(args, arg)
        PSLOT slot = NULL;
        // if we will be needing this more than once, and we should temporize it?
        if (!stack && maybeEnsureConstantStructValue(arg, &slot)) {
            // ok, it has been temporized, remember the slot number
            (singleArg ? addr->argSlot : addr->argSlots[count++]) = slot;
            // and if we need it on the stack now, dump it:
            if (dup | stackstore) {
                dumpLocal(slot, false);
            }
        } else {
            // is it definitely going to be on the stack? (ie, not stored)
            if (!store) {
                genExpr(arg);
            }
            // can we perhaps not store it if its constant?
            if (!stack && (arg->kind != EK_CONSTANT)) {
                // nope, we need it again and its not a constant
                if (store) {
                    // remember, if store was set we didn't gen it before, so do it now
                    genExpr(arg);
                } else {
                    // otherwise duplicate what's on the stack already
                    putOpcode(CEE_DUP);
                }
                // and save it in the approporiate slot
				TYPESYM * type = arg->type;
				if (isArray) {
					type = compiler()->symmgr.GetNaturalIntSym();
				}
                (singleArg ? addr->argSlot : addr->argSlots[count++]) = STOREINTEMP(type, TK_SHORTLIVED);
            }
        }
    ENDLOOP;

    if (!singleArg) {
        addr->stackCount = count;
    }

    if (dup) {
        dumpAddress(addr, false);
    }
}

// Determines whether the evaluation of e1 can be deferred until after the evaluation
// of e2
bool ILGENREC::canSwitchAround(EXPR * e1, EXPR * e2)
{
    createNewBitsetOnStack(read1, permLocalCount, 0);
    createNewBitsetOnStack(read2, permLocalCount, 0);
    createNewBitsetOnStack(written1, permLocalCount, 0);
    createNewBitsetOnStack(written2, permLocalCount, 0);

    bool isCall1 = false;
    bool isCall2 = false;
    bool isFieldDef1 = false;
    bool isFieldDef2 = false;
    bool isFieldUse1 = false;
    bool isFieldUse2 = false;

    analyzeExpression(e1, &read1, &written1, &isCall1, &isFieldDef1, &isFieldUse1);
    analyzeExpression(e2, &read2, &written2, &isCall2, &isFieldDef2, &isFieldUse1);

    if (isCall1 && (isCall2 || isFieldUse2)) return false;
    if (isCall2 && (isCall1 || isFieldUse1)) return false;
    if ((isFieldUse1 && isFieldDef2) || (isFieldUse2 && isFieldDef1)) return false;

    read1 = read1->andInto(written2);
    read2 = read2->andInto(written1);

    return read1->isZero() && read2->isZero();    
}

void ILGENREC::analyzeExpression(EXPR * expr, BITSET ** read, BITSET ** written, bool *isCall, bool *isFieldDef, bool *isFieldUse)
{

}


void ILGENREC::emitRefValue(EXPRBINOP * tree)
{
    genExpr(tree->p1);
    putOpcode(CEE_REFANYVAL);
    emitTypeToken(tree->p2->asTYPEOF()->sourceType);
}


// generate the address of an expression.  addr directs where the address
// should be stored...
void ILGENREC::genAddress(EXPR * tree, PADDRESS addr)
{
    EXPR * object;
    ARRAYSYM * arrayType;

#ifdef DEBUG
    int store = addr->type & ADDR_STORE;
#endif
    int stack = addr->type & ADDR_STACK;
    int dup = addr->type & ADDR_DUP;
    int stackstore = addr->type & ADDR_STACKSTORE;
    ASSERT(!stack || !dup);

    addr->expr = tree;
        
    switch (tree->kind) {
    case EK_WRAP:
        if (!tree->asWRAP()->slot) {
            tree->asWRAP()->slot = allocTemporary(tree->asWRAP()->type, tree->asWRAP()->tempKind);
        }
        goto NONE;
    case EK_LOCAL:
        ASSERT(!tree->asLOCAL()->local->isConst);
        if (tree->asLOCAL()->local->slot.isRefParam) {
            addr->stackCount = 0;
            if (stack | dup | stackstore) {
                dumpLocal(&(tree->asLOCAL()->local->slot), false);
                if (dup) {
                    putOpcode(CEE_DUP);
                }
            }
            return;
        } 
NONE:
        addr->type = ADDR_NONE;
        return;
    case EK_INDIR:
        genExpr(tree->asBIN()->p1);
        if (dup) {
            putOpcode(CEE_DUP);
        }
        addr->stackCount = 0;
        return;

    case EK_VALUERA:
        
        // these have been deprecated and should no longer be necessary...
        ASSERT(!store);
        ASSERT(!stackstore);

        emitRefValue(tree->asBIN());
        if (dup) {
            putOpcode(CEE_DUP);
        }
        addr->stackCount = 0;
        return;
    case EK_FIELD:
        object = tree->asFIELD()->object;
        if (tree->asFIELD()->field->isStatic) {
            ASSERT(!object);
            goto NONE;
        }
        genObjectPtr(object, tree->asFIELD()->field, addr);
        storeObjPointerAndArgs(tree->asFIELD()->object, NULL, addr);
        return;
    case EK_ARRINDEX:
        genExpr(tree->asBIN()->p1);
        if (tree->type->fundType() == FT_STRUCT) {
            genExpr(tree->asBIN()->p2);
            if ((arrayType = tree->asBIN()->p1->type->asARRAYSYM())->rank == 1) {
                putOpcode(CEE_LDELEMA);
                emitTypeToken(arrayType->elementType());
            } else {
                genArrayCall(arrayType, arrayType->rank ? arrayType->rank : getArgCount(tree->asBIN()->p2), ARRAYMETH_LOADADDR);
            }
            storeObjPointerAndArgs(tree->asBIN()->p1, NULL, addr);
        } else {
            storeObjPointerAndArgs(tree->asBIN()->p1, tree->asBIN()->p2, addr);
        }
        return;
    case EK_PROP:
        if (tree->asPROP()->prop->isStatic) {
            ASSERT(!tree->asPROP()->object);
        } else {
            genObjectPtr(tree->asPROP()->object, tree->asPROP()->prop, addr);
        }
        storeObjPointerAndArgs(tree->asPROP()->object, tree->asPROP()->args, addr);
        return;
    default:
        ASSERT(!"bad addr expr");
    }
}

__forceinline void ILGENREC::freeAddress(PADDRESS addr) 
{
    if (addr->type == ADDR_NONE) return;
    freeAddressMore(addr);

}

void ILGENREC::freeAddressMore(PADDRESS addr)
{
    if (addr->baseSlot) {
        freeTemporary(addr->baseSlot);
    }

    if (addr->stackCount == (unsigned) -1) {
        if (addr->argSlot) {
            freeTemporary(addr->argSlot);
        }
    } else {
        for (unsigned i = 0; i < addr->stackCount; i++) {
            freeTemporary(addr->argSlots[i]);
        }
    }
}


void ILGENREC::dumpAddress(PADDRESS addr, bool free) 
{
    if (addr->expr->kind == EK_LOCAL && addr->expr->asLOCAL()->local->slot.isRefParam) {
        dumpLocal(&(addr->expr->asLOCAL()->local->slot), false);
        return;
    }

    EXPR * object;
    if (addr->baseSlot) {
        dumpLocal(addr->baseSlot, false);
    } else if ((object = addr->expr->getObject())) {
        if (addr->type & ADDR_CONSTANT) {
            PSLOT slot = NULL;
            genMemoryAddress(addr->expr->getObject(), &slot);
            ASSERT(!slot);
            if (addr->type & ADDR_NEEDBOX) {
                ASSERT(!"why is NEED_BOX set when boxval is true?");

                putOpcode(CEE_BOX);
                emitTypeToken(object->type);
            }
        }
    }

    if (addr->stackCount | ((DWORD_PTR)(addr->argSlots))) {
        unsigned count = 0;
        // stackcount is -1, if singleArg case, so ~(-1) is 0 for the multiArgCase
        int multiArgs = ~(addr->stackCount);
        EXPRLOOP(addr->expr->getArgs(), arg)
            if (arg->kind == EK_CONSTANT) {
                genExpr(arg);
            } else {
                dumpLocal((multiArgs ? addr->argSlots[count++] : addr->argSlot), false);
            }
        ENDLOOP;
    }
}

void ILGENREC::genFlipParams(TYPESYM * elemType, unsigned count)
{
    curStack += count + 1;

    PSLOT slot = STOREINTEMP(elemType, TK_SHORTLIVED);
    PSLOT * slots = (PSLOT*) _alloca(sizeof(PSLOT) * count);
    unsigned i;
    for (i = 0; i < count; i++) {
        slots[i] = STOREINTEMP(compiler()->symmgr.GetPredefType(PT_INT, false), TK_SHORTLIVED);
    }
    dumpLocal(slot, false);
    freeTemporary(slot);
    for (i = count; i != 0; i--) {
        dumpLocal(slots[i-1], false);
        freeTemporary(slots[i-1]);
    }

    curStack -= count + 1;
}

void ILGENREC::genArrayCall(ARRAYSYM * array, int args, ARRAYMETHOD meth)
{
    curStack -= args; // (1 for obj pointer)

    ILCODE il = CEE_CALL;

    switch (meth) {
    case ARRAYMETH_LOAD:
    case ARRAYMETH_LOADADDR:
    case ARRAYMETH_GETAT:
        break;
    case ARRAYMETH_STORE:
        curStack -= 2;
        break;
    case ARRAYMETH_CTOR:
        il = CEE_NEWOBJ;
    default:
        break;
    }
    markStackMax();
    putOpcode(il);
    emitArrayMethodToken(array, meth);
}

// given an address on the stack, generate a load or a store to the value of the
// expression
void ILGENREC::genFromAddress(PADDRESS addr, bool store, bool free)
{
    ARRAYSYM * arrayType;
    EXPR * tree = addr->expr;
    LOCSLOTINFO * slot;
    switch (tree->kind) {
    case EK_FIELD:
        ILCODE ilcode;
        if (tree->asFIELD()->field->isStatic) {
            ilcode = store ? CEE_STSFLD : CEE_LDSFLD;
            ASSERT(addr->type == ADDR_NONE);
        } else {
            ilcode = store ? CEE_STFLD : CEE_LDFLD;
        }
        if (tree->asFIELD()->field->isVolatile)
            putOpcode(CEE_VOLATILE); // add volatile prefix when accessing volatile fields
        putOpcode(ilcode);
        emitFieldToken(tree->asFIELD()->field);
        break;
    case EK_VALUERA:
    case EK_INDIR:
        goto USEINDIRECTION;
    case EK_WRAP:
        ASSERT(tree->asWRAP()->slot);
        dumpLocal(tree->asWRAP()->slot, store);
        break;
    case EK_LOCAL: {
        ASSERT(!tree->asLOCAL()->local->isConst);
        slot = &(tree->asLOCAL()->local->slot);
        if (slot->isRefParam) {
USEINDIRECTION:
            FUNDTYPE ft = tree->type->fundType();
            ILCODE il = (store ? ILGENREC::ILstackStore : ILGENREC::ILstackLoad) [ft];
            putOpcode(il);
            if (ft == FT_STRUCT) {
                emitTypeToken(tree->type);
            }
        } else {
            ASSERT(addr->type == ADDR_NONE);
            dumpLocal(slot, store);
        }
        break;
        }
    case EK_ARRINDEX:
        arrayType = tree->asBIN()->p1->type->asARRAYSYM();
        if (tree->flags & EXF_ARRFLAT) {
            ASSERT(!store);
            genArrayCall(arrayType, 1, ARRAYMETH_GETAT); 
        } else {
            if (tree->type->fundType() == FT_STRUCT) {
                putOpcode(store ? CEE_STOBJ : CEE_LDOBJ);
                emitTypeToken(tree->type);
            } else {
                switch (arrayType->rank) {
                case 1:
                    putOpcode((store ? ILGENREC::ILarrayStore : ILGENREC::ILarrayLoad) [tree->type->fundType()]);
                    break;
                default:
                    genArrayCall(arrayType, arrayType->rank ? arrayType->rank : getArgCount(tree->asBIN()->p2), store ? ARRAYMETH_STORE : ARRAYMETH_LOAD);
                }
            }
        }
        break;
    case EK_PROP: 
        genPropInvoke(tree->asPROP(), addr, store);
        break;
    default:
        ASSERT(!"bad addr expr");
    }
    if (free) freeAddress(addr);
}


METHPROPSYM * ILGENREC::getVarArgMethod(METHPROPSYM * sym, EXPR * args)
{


    ASSERT(sym->isVarargs);
    unsigned realCount = getArgCount(args);

    unsigned fixedCount = sym->cParams - 1;

    ASSERT((int)realCount >= fixedCount);

    TYPESYM ** sig;

    if (realCount > fixedCount) {
        realCount++; // for the sentinel
    }

    sig = (TYPESYM**) _alloca(sizeof(PTYPESYM) * realCount);
    memcpy(sig, sym->params, sizeof(PTYPESYM) * fixedCount);

    if (realCount > fixedCount) {

        TYPESYM ** sigNew = sig + fixedCount;

        *sigNew = compiler()->symmgr.GetArglistSym();
        sigNew++;

        int originalCount = fixedCount;
        EXPRLOOP(args, arg)
            originalCount--;
            if (originalCount < 0) {
                if (arg->type == compiler()->symmgr.GetNullType()) {
                    *sigNew = compiler()->symmgr.GetPredefType(PT_OBJECT);
                } else {
                    *sigNew = arg->type;
                }
                sigNew++;
            }
        ENDLOOP;
    }

    sig = compiler()->symmgr.AllocParams(realCount, sig);

    SYMKIND kind = sym->kind;
    SYMKIND fakeKind = (SYMKIND)(kind + 1);
    ASSERT(fakeKind == SK_FAKEMETHSYM || fakeKind == SK_FAKEPROPSYM);
    int mask = 1 << fakeKind;

    METHPROPSYM * previous = compiler()->symmgr.LookupGlobalSym(sym->name, sym->parent, mask)->asMETHPROPSYM();

    while (previous && (*(previous->getFParentSym()) != sym || previous->retType != sym->retType || previous->params != sig)) {
        previous = compiler()->symmgr.LookupNextSym(previous, sym->parent, mask)->asMETHPROPSYM();
    }

    if (previous) return previous;

    previous = compiler()->symmgr.CreateGlobalSym(fakeKind, sym->name, sym->parent)->asMETHPROPSYM();

    ASSERT(kind == SK_METHSYM);
    sym->asMETHSYM()->copyInto(previous->asFMETHSYM());
    *(previous->getFParentSym()) = sym;
    previous->cParams = realCount;
    previous->params = sig;

    return previous;

}

// Helper for getBaseMethod. Searches all classes strictly BETWEEN the current class
// and sym->parent for a method/property with the same kind, name, signature, and return type as sym.
// If one or more is found, returns the class it was found in. If none is found, returns
// the current class.
AGGSYM * ILGENREC::searchInterveningClasses(METHPROPSYM * sym)
{
    AGGSYM * base; // class we're currently searching.
    AGGSYM * foundSoFar = cls;  // intervening class found so far.

    for (base = cls->baseClass; base != sym->parent; base = base->baseClass)
    {   
        METHPROPSYM *symFound = compiler()->symmgr.LookupGlobalSym(sym->name, base, 1 << sym->kind)->asMETHPROPSYM();
        while (symFound && 
               (symFound->params != sym->params || symFound->retType != sym->retType))
        {
            symFound = compiler()->symmgr.LookupNextSym(symFound, base, 1 << sym->kind)->asMETHPROPSYM();
        }

        if (symFound)
            foundSoFar = symFound->parent->asAGGSYM();
    }

    return foundSoFar;
}


// If we're doing a base.method() call, the sym passed in is the nearest symbol 
// in the hierarchy that matches. However, for versioning purposes, we want to work correctly
// if a new symbol is insert post-compiler "closer" to the call site. To do this, we insert
// a "fake" symbol to represent the pseudo-method we will call. If all intermediate classes
// are in our output file, though, this isn't needed.
METHPROPSYM * ILGENREC::getBaseMethod(METHPROPSYM * sym)
{
    AGGSYM * base; // the class we'll create the fake method on.
    OUTFILESYM * myOutfile = cls->getInputFile()->getOutputFile();

    // Search upward until we hit either a class defined outside our output file,
    // or we hit the "real" method.
    base = searchInterveningClasses(sym);
    do {
        if (base->baseClass == sym->parent) 
            return sym;
        base = base->baseClass;
    } while (base->getInputFile()->getOutputFile() == myOutfile);

    // We hit a class defined outside our output file first. Place the "fake" method on that class.

    SYMKIND kind = sym->kind;
    SYMKIND fakeKind = (SYMKIND)(kind + 1);
    int mask = 1 << fakeKind;

    ASSERT(fakeKind == SK_FAKEMETHSYM || fakeKind == SK_FAKEPROPSYM);

    METHPROPSYM * previous;

    previous = compiler()->symmgr.LookupGlobalSym(sym->name, base, mask)->asMETHPROPSYM();

    while (previous && !*(previous->getFParentSym()) && (previous->retType != sym->retType || previous->params != sym->params)) {
        previous = compiler()->symmgr.LookupNextSym(previous, base, mask)->asMETHPROPSYM();
    }

    if (previous) return previous;

    previous = compiler()->symmgr.CreateGlobalSym(fakeKind, sym->name, base)->asMETHPROPSYM();

    if (fakeKind == SK_FAKEMETHSYM) {
        sym->asMETHSYM()->copyInto(previous->asFAKEMETHSYM());
    } else {
        sym->asPROPSYM()->copyInto(previous->asFAKEPROPSYM());
        if (sym->asPROPSYM()->methGet) {
            previous->asFAKEPROPSYM()->methGet = compiler()->symmgr.CreateGlobalSym(SK_FAKEMETHSYM, sym->asPROPSYM()->methGet->name, base)->asFAKEMETHSYM();
            sym->asPROPSYM()->methGet->copyInto(previous->asFAKEPROPSYM()->methGet);
        }
        if (sym->asPROPSYM()->methSet) {
            previous->asFAKEPROPSYM()->methSet = compiler()->symmgr.CreateGlobalSym(SK_FAKEMETHSYM, sym->asPROPSYM()->methSet->name, base)->asFAKEMETHSYM();
            sym->asPROPSYM()->methSet->copyInto(previous->asFAKEPROPSYM()->methSet);
        }
    }
    return previous;

}


void ILGENREC::genPropInvoke(EXPRPROP* tree, PADDRESS addr, bool store)
{
    METHSYM *meth;
    int baseCall = tree->flags & EXF_BASECALL;

    PROPSYM * prop;

    if (baseCall) {
        prop = getBaseMethod(tree->prop)->asFPROPSYM();
    } else {
        prop = tree->prop;
    }

    if (store) {
        meth = prop->methSet;
    } else {
        meth = prop->methGet;
        curStack ++;
        markStackMax();
    }
    ASSERT(meth);

#if USAGEHACK
    meth->isUsed = true;
#endif

    ILCODE il = (callAsVirtual(meth, tree->object, !!baseCall)) ? CEE_CALLVIRT : CEE_CALL;

    putOpcode(il);

    emitMethodToken(meth);

    curStack -= meth->cParams;

    if (!meth->isStatic) curStack --;

}

__forceinline PSLOT ILGENREC::storeLocal(PSLOT slot)
{
    dumpLocal(slot, true);
    return slot;
}

// store or load a local or param.  we try to use the smallest
// opcode available...
void ILGENREC::dumpLocal(LOCSLOTINFO * slot, int store)
{
    ASSERT(store == 0 || store == 1);

    bool byValue = true;
    ILCODE ilcode;

    if (byValue) {
        unsigned islot = slot->ilSlot;

        int idx1 = (slot->isParam ? 2 : 0) + store;
        if (islot < 4) {
            ilcode = ILGENREC::ILlsTiny[idx1][islot];
            if (ilcode == CEE_ILLEGAL) goto USE_S;
            putOpcode(ilcode);
        } else if (islot <= 0xFF) {
USE_S:
            putOpcode(ILGENREC::ILlsTiny[idx1][4]);
            putCHAR((char)islot);
        } else {
            ASSERT(islot <= 0xffff);
            putOpcode(ILGENREC::ILlsTiny[idx1][5]);
            putWORD((WORD)islot);
        }
    }
    if (!store && slot->isPinned) {
        putOpcode(CEE_CONV_I);
    }
}

#if DEBUG
// verify that all temporaries have been deallocated
void ILGENREC::verifyAllTempsFree()
{
    TEMPBUCKET * bucket = temporaries;
    while (bucket) {
        for (unsigned i = 0; i < TEMPBUCKETSIZE ; i++) {
            ASSERT(! // Its easier to select if its on its own line...
                bucket->slots[i]
                .isTaken || bucket->slots[i].isDeleted);
        }
        bucket = bucket ->next;
    }
}

// allocate a new temporary, and record where from when in debug
#undef allocTemporary
#undef allocTemporaryRef
PSLOT ILGENREC::allocTemporary(TYPESYM * type, TEMP_KIND tempKind, char *file, unsigned line)
#else
PSLOT ILGENREC::allocTemporary(TYPESYM * type, TEMP_KIND tempKind)
#endif
{
    ASSERT(tempKind != TK_REUSABLE);

    if (type->kind == SK_NULLSYM) {
        type = compiler()->symmgr.GetPredefType(PT_OBJECT);
    }

    int i = 0;
    unsigned bucketCount = 0;
    TEMPBUCKET * bucket = temporaries;
    do {
        if (!bucket) { 
            bucket = (TEMPBUCKET *) allocator->AllocZero(sizeof(TEMPBUCKET));
            if (!temporaries) {
                temporaries = bucket;
            } else {
                TEMPBUCKET * temp = temporaries;
                while (temp->next) temp = temp->next;
                temp->next = bucket;
            }
        }
        if ((!bucket->slots[i].type) || 
            ((!bucket->slots[i].isTaken) && bucket->slots[i].type == type && 
                    (bucket->slots[i].tempKind == TK_REUSABLE || (bucket->slots[i].tempKind == tempKind && bucket->slots[i].canBeReusedInEnC)))) {
            bucket->slots[i].isTaken = true;

            if (!bucket->slots[i].canBeReusedInEnC && 0 == bucket->slots[i].ilSlot) {
                bucket->slots[i].ilSlot = permLocalCount + i + bucketCount * TEMPBUCKETSIZE;
            }

            if (compiler()->options.m_fEMITDEBUGINFO && !compiler()->options.m_fOPTIMIZATIONS) {
                bucket->slots[i].tempKind = tempKind;
                bucket->slots[i].canBeReusedInEnC = false;
            } else {
                ASSERT(bucket->slots[i].tempKind == TK_REUSABLE);
            }

            bucket->slots[i].isTemporary = true;
            bucket->slots[i].type = type;
#if DEBUG
            bucket->slots[i].lastFile = file;
            bucket->slots[i].lastLine = line;
#endif
            return &(bucket->slots[i]);
        }
        i++;
        if (i == TEMPBUCKETSIZE) {
            i = 0;
            bucket = bucket->next;
            bucketCount ++;
        }
    } while (true);
}

PSLOT ILGENREC::allocTemporaryRef(TYPESYM * type, TEMP_KIND tempKind
#if DEBUG
                                  ,char *file, unsigned line
#endif
                                  )
{
    if (type->isStructType()) {
        type = compiler()->symmgr.GetParamModifier(type, false);
    }
#if DEBUG
    return allocTemporary(type, tempKind, file, line);
#else 
    return allocTemporary(type, tempKind);
#endif
}

// reset the define now that we defined the functions
#if DEBUG
#define allocTemporary(t, k) allocTemporary(t, k, __FILE__, __LINE__)
#define allocTemporaryRef(t, k) allocTemporaryRef(t, k, __FILE__, __LINE__)
#endif


// free the given temporary
__forceinline void ILGENREC::freeTemporary(PSLOT slot)
{
    ASSERT(slot->isTaken && !slot->isParam && slot->type && slot->isTemporary && !slot->isDeleted);
    slot->isTaken = false;
}


// emit MD tokens of various types
void ILGENREC::emitFieldToken(MEMBVARSYM * sym)
{
    mdToken tok = compiler()->emitter.GetMembVarRef(sym);
#if DEBUG
//    ASSERT(!inlineBB.sym);
    inlineBB.sym = sym;
#endif
    putDWORD(tok);
}

void ILGENREC::emitMethodToken(METHSYM * sym)
{
    mdToken tok = compiler()->emitter.GetMethodRef(sym);
#if DEBUG
    inlineBB.sym = sym;
#endif
    putDWORD(tok);
}

void ILGENREC::emitTypeToken(TYPESYM * sym)
{
    mdToken tok = compiler()->emitter.GetTypeRef(sym);
#if DEBUG
    inlineBB.sym = sym;
#endif
    putDWORD(tok);
}

void ILGENREC::emitArrayMethodToken(ARRAYSYM * sym, ARRAYMETHOD methodId)
{
    mdToken tok = compiler()->emitter.GetArrayMethodRef(sym, methodId);
#if DEBUG
    inlineBB.sym = sym;
#endif
    putDWORD(tok);
}

// generate the sideeffects of an expression
void ILGENREC::genSideEffects(EXPR * tree)
{
    EXPRBINOP * colon;
    BBLOCK * labEnd;

AGAIN:

    if (!tree) return;

    if (tree->flags & (EXF_ASSGOP | EXF_CHECKOVERFLOW)) {
        genExpr(tree, NULL);
        return;
    }
    if (tree->flags & EXF_BINOP) {
        bool sense;
        switch (tree->kind) {
        case EK_IS:
        case EK_AS:
        case EK_SWAP:
            genSideEffects(tree->asBIN()->p1);
            tree = tree->asBIN()->p2;
            goto AGAIN;
        case EK_LOGAND: 
            sense = false;
            // for (a && b) and (a || b) we generate a and then b depending on the 
            // result of a.

            // if b has no sideefects, then we generate se(a)
DOCONDITIONAL:

            if (tree->asBIN()->p2->hasSideEffects(compiler())) {
                labEnd = genCondBranch(tree->asBIN()->p1, createNewBB(), sense);
                genSideEffects(tree->asBIN()->p2);
                startNewBB(labEnd);
                return;
            } else {
                tree = tree->asBIN()->p1;
                goto AGAIN;
            }
                        
        case EK_LOGOR: 
            sense = true;
            goto DOCONDITIONAL;

        case EK_QMARK:
            colon = tree->asBIN()->p2->asBINOP();
            if ((colon->p1 && colon->p1->hasSideEffects( compiler())) ||
                (colon->p2 && colon->p2->hasSideEffects( compiler()))) {
                BBLOCK * labTrue = genCondBranch(tree->asBIN()->p1, createNewBB(), true);
                BBLOCK * labFallThrough = createNewBB();
                genSideEffects(colon->p2);
                startNewBB(labTrue, CEE_BR, labFallThrough);
                genSideEffects(colon->p1);
                startNewBB(labFallThrough);
                return;
            } else {
                tree = tree->asBIN()->p1;
                goto AGAIN;
            }
	default:
	    break;
        }

        genSideEffects(tree->asBIN()->p1);
        tree = tree->asBIN()->p2;
        goto AGAIN;
    }

    switch (tree->kind) {
    case EK_ZEROINIT:
        if (tree->hasSideEffects( compiler())) {
            genZeroInit(tree->asZEROINIT(), NULL);
        }
        break;
    case EK_CONCAT:
    case EK_PROP:
        genExpr(tree, NULL);
        break;
    case EK_CALL:
        genCall(tree->asCALL(), NULL);
        break;
    case EK_NOOP:
        break;
    case EK_FIELD:
        if (tree->hasSideEffects( compiler())) {
            genExpr(tree, NULL);
            return;
        } else {
            tree = tree->asFIELD()->object;
            goto AGAIN;
        }
    case EK_ARRINIT:
        tree = tree->asARRINIT()->args;
        goto AGAIN;
    case EK_CAST:
        if (tree->flags & (EXF_BOX | EXF_UNBOX | EXF_CHECKOVERFLOW | EXF_REFCHECK)) {
            genExpr(tree, NULL);
        } else {
            tree = tree->asCAST()->p1;
            goto AGAIN;
        }
        // fall through otherwise...
    case EK_LOCAL:
    case EK_CONSTANT:
    case EK_FUNCPTR:
    case EK_TYPEOF:
    case EK_WRAP:
        return;
    default:
        ASSERT(!"bad expr");
    }
}

void ILGENREC::genSlotAddress(PSLOT slot, bool ptrAddr)
{

    if (slot->isRefParam || (!ptrAddr && (slot->type && slot->type->kind == SK_PTRSYM))) {
        dumpLocal(slot, false);
        return;
    }

    int size = 1;
    if (((BYTE)(slot->ilSlot & 0xff)) == slot->ilSlot) {
        size = 0; // so its small
    }
    putOpcode(ILaddrLoad[slot->isParam][size]);
    if (size) {
        putWORD((WORD)slot->ilSlot);
    } else {
        putCHAR((char)slot->ilSlot);
    }
}


void ILGENREC::genSizeOf(TYPESYM * typ)
{
    putOpcode(CEE_SIZEOF);
    emitTypeToken(typ);
}


void ILGENREC::genMemoryAddress(EXPR * tree, PSLOT * pslot, bool ptrAddr)
{

    TYPESYM * type;

    switch (tree->kind) {
    case EK_WRAP:
        if (!tree->asWRAP()->slot) {
            if (tree->asWRAP()->needEmptySlot) {
                tree->asWRAP()->slot = allocTemporary(tree->type, tree->asWRAP()->tempKind);
            } else {
                // we need to get the thing from the stack and load its address
                tree->asWRAP()->slot = STOREINTEMP(tree->type, tree->asWRAP()->tempKind);
                *pslot = tree->asWRAP()->slot;
            }
        }
        genSlotAddress(tree->asWRAP()->slot);
        return;
    case EK_LOCAL:
        ASSERT(!tree->asLOCAL()->local->isConst);
        genSlotAddress(&(tree->asLOCAL()->local->slot), ptrAddr);
        return;
    case EK_VALUERA:
        emitRefValue(tree->asBIN());
        return;
    case EK_INDIR:
        genExpr(tree->asBIN()->p1);
        return;
    case EK_FIELD:
        if (!(tree->flags & EXF_LVALUE) /* || tree->asFIELD()->field->parent->asAGGSYM()->isMarshalByRef */ ) {   
            ASSERT(!ptrAddr);
            genExpr(tree);
            *pslot = STOREINTEMP(tree->type, TK_SHORTLIVED);
            genSlotAddress(*pslot);
            return;
        }
        if (tree->asFIELD()->field->isStatic) {
            ASSERT(!tree->asFIELD()->object);
            putOpcode(CEE_LDSFLDA);
            ASSERT(! tree->asFIELD()->field->isVolatile); // Should never access volatile field by address.
        } else {
            genObjectPtr(tree->asFIELD()->object, tree->asFIELD()->field, pslot);
            if (!ptrAddr && tree->asFIELD()->object->type->kind == SK_PTRSYM) {
                if (tree->asFIELD()->field->isVolatile)
                    putOpcode(CEE_VOLATILE);
                putOpcode(CEE_LDFLD);
            } else {
                putOpcode(CEE_LDFLDA);
                ASSERT(! tree->asFIELD()->field->isVolatile); // Should never access volatile field by address.
            }
        }
        emitFieldToken(tree->asFIELD()->field);
        return;
    case EK_ARRINDEX:
        genExpr(tree->asBIN()->p1);
        genExpr(tree->asBIN()->p2);
        if ((type = tree->asBIN()->p1->type)->asARRAYSYM()->rank == 1) {
            putOpcode(CEE_LDELEMA);
            emitTypeToken(type->asARRAYSYM()->elementType());
        } else {
            genArrayCall(type->asARRAYSYM(), type->asARRAYSYM()->rank ? type->asARRAYSYM()->rank : getArgCount(tree->asBIN()->p2), ARRAYMETH_LOADADDR);
        }
        return;
    default:
        type = tree->type;
        if (type->kind == SK_PARAMMODSYM) {
            type = type->asPARAMMODSYM()->paramType();
            ASSERT(tree->kind == EK_PROP);
        }
        genExpr(tree);
        *pslot = allocTemporary(type, TK_SHORTLIVED);
        dumpLocal(*pslot, true);
        genSlotAddress(*pslot);
    }
}


bool ILGENREC::maybeEnsureConstantStructValue(EXPR * arg, PSLOT * pslot)
{
    if (arg->type->fundType() != FT_STRUCT) return false;

    // In general, the only thing which needs to ensured is things whose value may
    // change...  we can easily tell which things these are because they have
    // another name for themselves:  LVALUES

    if (!(arg->flags & EXF_LVALUE)) return false;

    ensureConstantStructValue(arg, pslot);

    return true;

}

void ILGENREC::ensureConstantStructValue(EXPR * arg, PSLOT * pslot)
{
    ASSERT(arg->flags & EXF_LVALUE);

    genExpr(arg);

    // ok, the struct is now on the stack... store it in a temp...

    *pslot = STOREINTEMP(arg->type, TK_SHORTLIVED);

}

bool ILGENREC::hasConstantAddress(EXPR * expr)
{
AGAIN:
    switch(expr->kind) {
    case EK_LOCAL:
        return true;
    case EK_FIELD:
        if (expr->asFIELD()->field->isStatic) return true;
        if (expr->asFIELD()->object->type->asAGGSYM()->isStruct) {
            expr = expr->asFIELD()->object;
            goto AGAIN;
        }
    default:        
        return false;
    }
}

void ILGENREC::genObjectPtr(EXPR * object, SYM * member, ADDRESS * addr)
{
    
    unsigned needBox;

    if (object->type->isStructType() && !(addr->type & ADDR_LOADONLY))  {
        needBox = needsBoxing(member->parent, object->type);

        if (needBox) {
            genExpr(object);

            putOpcode(CEE_BOX);
            emitTypeToken(object->type);

            return;
            
        }


        int needOnStack = addr->type & (ADDR_STACK | ADDR_DUP | ADDR_STACKSTORE);

        // If it's STACK only, then we don't care whether it is or isnt a constant addr,
        // But, if its needed again, then we need to find out whether it is a constant addr
        // so that we won't be storing it unnecessarily...
        bool constAddr = (addr->type & (ADDR_DUP | ADDR_STACKSTORE | ADDR_STORE)) ? hasConstantAddress(object) : false;


        if (constAddr) {
            addr->type |= ADDR_CONSTANT;
        }

        // do we need to have anyting on the stack now?
        // we do, if its expected, or if it can change in the future...

        // Note that we defer boxing constant addresses if we can.

        if (needOnStack || !constAddr) {
            // we need to get the address of the struct onto the stack, instead of the
            // struct itself...

            genMemoryAddress(object, &(addr->baseSlot));

            if (needBox) {
        
                FREETEMP(addr->baseSlot);

                ASSERT(!member->parent->asAGGSYM()->isStruct);

                // We need to box the address...
                putOpcode(CEE_BOX);
                emitTypeToken(object->type);
            }
        } else {
            // remember to box it later...
            if (needBox) {
                ASSERT(!"cannot box later");
                addr->type |= ADDR_NEEDBOX;
            }

        }

    } else {
        genExpr(object);
    }
}

__forceinline unsigned ILGENREC::needsBoxing(PARENTSYM * parent, TYPESYM * object)
{
    ASSERT(object->isStructType());
    return parent != object;
}


void ILGENREC::genObjectPtr(EXPR * object, SYM * member, PSLOT * pslot)
{
    ASSERT(object);
    if (object->type->isStructType()) {

        // we need to get the address of the struct onto the stack, instead of the
        // struct itself...


        // unless we are boxing
        if (needsBoxing(member->parent, object->type)) {
            ASSERT(!member->parent->asAGGSYM()->isStruct);

            // We need to box the address...
            genExpr(object);
            putOpcode(CEE_BOX);
            emitTypeToken(object->type);
        } else {

            genMemoryAddress(object, pslot);
        }

    } else {
        genExpr(object);
    }
}


void ILGENREC::emitRefParam(EXPR * arg, PSLOT * curSlot)
{

    if (arg->kind == EK_LOCAL && !arg->asLOCAL()->local->slot.type) {
        curSlot[0] = allocTemporary(arg->type->parent->asTYPESYM(), TK_SHORTLIVED);
        initLocal(curSlot[0], arg->type->parent->asTYPESYM());
        genSlotAddress(curSlot[0]);
    } else {
        PSLOT slot = NULL;
        *curSlot = NULL;
        genMemoryAddress(arg, &slot, true);
        ASSERT(!slot);
    }
}



// Should we use the "call" or "callvirt" opcode? We use "callvirt" even on non-virtual
// isntance methods (exception base calls or structs) to get a null check.
bool ILGENREC::callAsVirtual(METHSYM * meth, EXPR * object, bool isBaseCall)
{
    if (!meth->isStatic && !isBaseCall && object->type->fundType() == FT_REF) {
        // If it's a non-virtual method, and we KNOW the reference can't be null, then 
        // use a non-virtual call anyway, though.
        if (!meth->isVirtual && (object->flags & EXF_CANTBENULL))
            return false;
        else
            return true;
    }
    else
        return false;
}
                         

// generate code for a function call
void ILGENREC::genCall(EXPRCALL * tree, PVALUSED valUsed)
{
    METHSYM * func = tree->method;

#if USAGEHACK
    func->isUsed = true;
#endif

    EXPR * object = tree->object;

    int isNewObjCall = tree->flags & EXF_NEWOBJCALL;
    bool isBaseCall = !!(tree->flags & EXF_BASECALL);
    int isStructAssgCall = tree->flags & EXF_NEWSTRUCTASSG;
    int hasRefParams = tree->flags & EXF_HASREFPARAM;
    bool needsCopy = false;
    bool dumpAddress = false;

    if (object && object->kind == EK_FIELD && object->asFIELD()->field->isReadOnly && !(object->flags & EXF_LVALUE)) {
        needsCopy = true;
    }

    if (isBaseCall) {
        func = getBaseMethod(func)->asFMETHSYM();
    }
    if (func->isVarargs) {
        func = getVarArgMethod(func, tree->args)->asFMETHSYM();
    }

    PSLOT slotObject = NULL;

    bool retIsVoid = tree->type == compiler()->symmgr.GetVoid();
    if (func->isStatic || isNewObjCall) {
        ASSERT(!object);

    } else {
        if (isStructAssgCall) {
            if (!isExprOptimizedAway(object)) {
                genObjectPtr(object, func, &slotObject);
                retIsVoid = true;
                if (valUsed) {
                    ASSERT(object);
                    if (object->kind == EK_LOCAL) {
                        // this is a problem... the verifier might complain that we are duping an
                        // uninitialized local's address                        
                        dumpAddress = true;
                    } else {
                        putOpcode(CEE_DUP);
                    }
                }
            } else {
                // if this is an assignment to a local which is not used, then don't assign...
                object = NULL;
                isNewObjCall = 1;
                isStructAssgCall = 0;
            }
        } else {
            genObjectPtr(object, func, &slotObject);
        }
    }

    int stackPrev = curStack;

    PSLOT * tempSlots = NULL;
    unsigned int slotCount = 0;

    if (hasRefParams) {

        PSLOT * curSlot = tempSlots = (PSLOT *) _alloca(sizeof(PSLOT) * func->cParams);
    
        EXPRLOOP(tree->args, arg)
            if (arg->type->kind == SK_PARAMMODSYM) {
                emitRefParam(arg, curSlot);
            } else {
                genExpr(arg);
                *curSlot = NULL;
            }
            curSlot++;
        ENDLOOP;

        slotCount = (unsigned)(curSlot - tempSlots);

    } else {

        genExpr(tree->args);
    }

    int stackDiff;

    if (!func->isVarargs) {
        stackDiff = func->cParams;
    } else {
        stackDiff = curStack - stackPrev;
    }

    if (!func->isStatic && !isNewObjCall) stackDiff ++;  // adjust for this pointer

    ILCODE ilcode;
    if (isNewObjCall) {
        ilcode = CEE_NEWOBJ;
    } else if (callAsVirtual(func, object, isBaseCall)) {
        ilcode = CEE_CALLVIRT;
    } else {
        ilcode = CEE_CALL;
    }

    putOpcode(ilcode);

    emitMethodToken(func);

    curStack -= stackDiff;
    // eat the arguments off the stack

    if (func->retType != compiler()->symmgr.GetVoid()) {
        curStack++;
        markStackMax();
    }
    // The difference between the above check (of funcrettype) and here, 
    // (of tree type) is that call exprs have a type of non-void for new_obj calls
    // since they denote a value of a certain type, while the constructor itself has
    // a return type of void...
    if (isStructAssgCall && valUsed) {
        if (dumpAddress) {
            genSlotAddress(&object->asLOCAL()->local->slot);
        }
        putOpcode(CEE_LDOBJ);
        emitTypeToken(object->type);
    } else if (!valUsed) {
        // so, if val is not used, and we had something, then we need to pop it...
        if (!retIsVoid) {
            putOpcode(CEE_POP);
        }
    }

    if (slotObject) {
        freeTemporary(slotObject);
    }

    if (tempSlots) {
        for (unsigned i = 0; i < slotCount; i++) {
            if (tempSlots[i]) {
                freeTemporary(tempSlots[i]);
            }
        }
    }

}


void ILGENREC::copyHandlers(COR_ILMETHOD_SECT_EH_CLAUSE_FAT * clauses)
{
    for (HANDLERINFO * hi = handlers;hi; hi = hi->next) {
        // is this protected block even reachable? 
        if (!hi->tryBegin) {          
            ehCount--;
            continue;
        }
        clauses->SetTryOffset(hi->tryBegin->startOffset);
        clauses->SetTryLength(hi->tryEnd->next->startOffset - clauses->GetTryOffset());
        clauses->SetHandlerOffset(hi->handBegin->startOffset);
        clauses->SetHandlerLength(hi->handEnd->next->startOffset - clauses->GetHandlerOffset());
        if (hi->type) {
            clauses->SetFlags(COR_ILEXCEPTION_CLAUSE_OFFSETLEN);
            clauses->SetClassToken(compiler()->emitter.GetTypeRef(hi->type));
        } else {
            clauses->SetFlags((CorExceptionFlag) (COR_ILEXCEPTION_CLAUSE_FINALLY | COR_ILEXCEPTION_CLAUSE_OFFSETLEN));
            clauses->SetClassToken(NULL);
        }

        clauses++;
    }
}


HANDLERINFO * ILGENREC::createHandler(BBLOCK * tryBegin, BBLOCK * tryEnd, BBLOCK * handBegin, TYPESYM * type)
{
    ehCount++;
    HANDLERINFO * handler = (HANDLERINFO*)allocator->Alloc(sizeof(HANDLERINFO));
    handler->tryBegin = tryBegin;
    handler->tryEnd = tryEnd;
    handler->handBegin = handBegin;
    handler->type = type;
    handler->next = NULL;

    if (lastHandler) {
        lastHandler->next = handler;
        lastHandler = handler;
    } else {
        lastHandler = handlers = handler;
    }

    return handler;

}

void ILGENREC::genTry(EXPRTRY * tree)
{

    if (tree->flags & EXF_REMOVEFINALLY) {
        ASSERT(tree->flags & EXF_ISFINALLY);
        genBlock(tree->tryblock);
        genBlock(tree->handlers->asBLOCK());
        return;
    }

    // start : is top of the try block
    BBLOCK * tryBegin = startNewBB(NULL);
    inlineBB.v.startsTry = true;
    genBlock(tree->tryblock);

    // end is after the end of the protected block
    BBLOCK * tryEnd;
    if (tree->flags & EXF_ISFINALLY) {
        BBLOCK * afterFinally = createNewBB();

        tryEnd = startNewBB(NULL); // this will point to the CEE_LEAVE...

        inlineBB.v.mayRemove = true; // this is a synthetic goto, which may point beyond the end of code... 

        if (tree->flags & EXF_FINALLYBLOCKED) {
            inlineBB.v.gotoBlocked = true; // this refers to the following leave
            blockedLeave++;
        }

        closeDebugInfo();  // close the info for the try block
        
        openDebugInfo(-1, EXF_NODEBUGINFO);
        BBLOCK * handBegin = startNewBB(NULL, CEE_LEAVE, afterFinally);
        closeDebugInfo();
        // and this will point to the finally block following the CEE_LEAVE...

        inlineBB.v.startsCatchOrFinally = true;
        genBlock(tree->handlers->asBLOCK());

        HANDLERINFO * hand = createHandler(tryBegin, tryEnd, handBegin, NULL);
        
        hand->handEnd = startNewBB(NULL);
        // this points to the CEE_ENDFINALLY...
    
        openDebugInfo(-1, EXF_NODEBUGINFO);
        inlineBB.v.endsFinally = true;
        putOpcode(CEE_ENDFINALLY);
        closeDebugInfo();
        startNewBB(afterFinally, CEE_ENDFINALLY);
    } else {

        // fallthrough is after the handler
        BBLOCK * fallThrough = createNewBB();

        tryEnd = startNewBB(NULL);
        // this points to the CEE_LEAVE

        inlineBB.v.mayRemove = true; 

        // close the info for the try block
        closeDebugInfo();  

        openDebugInfo(-1, EXF_NODEBUGINFO);
        startNewBB(NULL, CEE_LEAVE, fallThrough);
        closeDebugInfo();
        // Current is now after the CEE_LEAVE...

        EXPRLOOP(tree->handlers, hand)
            EXPRHANDLER * catchHandler = hand->asHANDLER();
            BBLOCK * handBegin = currentBB;
            inlineBB.v.startsCatchOrFinally = true;

            curStack++;
            markStackMax();

            openDebugInfo(catchHandler->tree, 0);  // emit debug info for the catch.

            bool usedParam = catchHandler->param && catchHandler->param->slot.type;
            if (usedParam) {
                maybeEmitDebugLocalUsage(catchHandler->tree, catchHandler->param);
                dumpLocal(&(catchHandler->param->slot), true);
            } else {
                putOpcode(CEE_POP);
            }

            closeDebugInfo();  // finish debug info for the catch.

            genBlock(catchHandler->handlerBlock);

            HANDLERINFO * handler = createHandler(tryBegin, tryEnd, handBegin, catchHandler->type);
            handler->handEnd = startNewBB(NULL);
            // this will now point to the CEE_LEAVE...

            inlineBB.v.mayRemove = true;  

            closeDebugInfo();  // finish debug info for the catch.

            openDebugInfo(-1, EXF_NODEBUGINFO);
            startNewBB(NULL, CEE_LEAVE, fallThrough);
            closeDebugInfo();
        ENDLOOP;

        startNewBB(fallThrough);
    }
}


void ILGENREC::genThrow(EXPRTHROW * tree)
{
    if (tree->object) {
        genExpr(tree->object);
        putOpcode(CEE_THROW);
    } else {
        putOpcode(CEE_RETHROW);
    }
    startNewBB(NULL, cee_stop);
}

__forceinline unsigned ILGENREC::computeSwitchSize(BBLOCK * block)
{
    ASSERT(block->exitIL == CEE_SWITCH);

    return ILcodesSize[CEE_SWITCH] + 4 + (4 * block->switchDest->count);
}

// calculate the size of our code and set the correct bb offsets.
unsigned ILGENREC::getFinalCodeSize()
{

    if (returnLocation && !returnHandled) {
        startNewBB(returnLocation);
        openDebugInfo(getCloseIndex(), 0);
        if (retTemp) {
            dumpLocal(retTemp, false);
            freeTemporary(retTemp);
        }
        putOpcode(CEE_RET);
        closeDebugInfo();
    } else if (retTemp) {
        freeTemporary(retTemp);
    }

    endBB(cee_stop, NULL);
    currentBB->next = NULL;

    suckUpGotos();

    size_t curoffset = 0;
    BBLOCK * curBB = firstBB;

    do {
        curBB->startOffset = (unsigned) curoffset;
        curBB->u.largeJump = 0;
        curoffset += curBB->curLen;
        switch(curBB->exitIL) {
        case CEE_RET:
        case cee_stop:
        case CEE_ENDFINALLY:
        case cee_next: 
            break;
        case CEE_SWITCH:
            curoffset += computeSwitchSize(curBB);
            break;
        default:
            ASSERT(curBB->exitIL < cee_last);
            curBB->u.largeJump = 1;
            // assume a large offset...
            curoffset += ILcodesSize[curBB->exitIL] + 4;
        }
        curBB = curBB->next;
    } while (curBB);

    unsigned delta;
    do {
        curBB = firstBB;
        delta = 0;
        do {
            curBB->startOffset -= delta;
            if (curBB->u.largeJump) {
                int offset;
                ILCODE newOpcode = getShortOpcode(curBB->exitIL);
                if (curBB->jumpDest->startOffset > curBB->startOffset) {
                    // forward jump
                    offset = (int)(curBB->jumpDest->startOffset - curBB->startOffset -
                        delta - ILcodesSize[curBB->exitIL] - 4 - curBB->curLen);
                } else {
                    // backward jump
                    offset = (int)(curBB->jumpDest->startOffset - 
                        (curBB->startOffset + curBB->curLen + ILcodesSize[newOpcode] + 1));

                }
                if (offset == (char) (offset & 0xff)) {
                    // this fits!!!
                    delta += (ILcodesSize[curBB->exitIL] - ILcodesSize[newOpcode]) + 3;
                    curBB->exitIL = newOpcode;
                    curBB->u.largeJump = false;
                }
            }
            curBB = curBB->next;
        } while (curBB);
        curoffset -= delta;
    } while (delta);

    #if DEBUG
        if (compiler()->GetRegDWORD("After")) {
            dumpAllBlocksContent();
        }
    #endif
    

    return (unsigned)curoffset;
}

BYTE * ILGENREC::copySwitchInstruction(BYTE * outBuffer, BBLOCK * block)
{
    ASSERT(block->exitIL == CEE_SWITCH);

    putOpcode(&outBuffer, CEE_SWITCH);

    SET_UNALIGNED_VAL32(outBuffer, block->switchDest->count);
    outBuffer += sizeof(DWORD);

    unsigned instrSize = computeSwitchSize(block);

    for (unsigned i= 0; i < block->switchDest->count; i++) {
        SET_UNALIGNED_VAL32(outBuffer, computeJumpOffset(block, block->switchDest->blocks[i].dest, instrSize));
        outBuffer += sizeof(int);
    }

    return outBuffer;
}

int ILGENREC::computeJumpOffset(BBLOCK * from, BBLOCK * to, unsigned instrSize)
{
    int offset;
    if (to->startOffset > from->startOffset) {
        // forward jump
        offset = (int)(to->startOffset - from->startOffset - instrSize - from->curLen);
    } else {
        // backward jump
        offset = (int)(to->startOffset - (from->startOffset + from->curLen + instrSize));
    }
    return offset;
}

// copy the code to the given buffer.  deltaToCode gives the speace taken up
// by the method header.  return the adjused buffer pointer
BYTE * ILGENREC::copyCode(BYTE * outBuffer, unsigned deltaToCode)
{
    for (BBLOCK * curBB = firstBB;curBB;curBB = curBB->next) {
        ASSERT(curBB->reachable || !compiler()->options.m_fOPTIMIZATIONS || (curBB->curLen == 0 && (curBB->exitIL == cee_next || curBB->exitIL == cee_stop)));
        if (!curBB->reachable) continue;

        memcpy(outBuffer, curBB->code, curBB->curLen);
        outBuffer += curBB->curLen;

        switch (curBB->exitIL) {
        case CEE_RET:
        case cee_stop:
        case CEE_ENDFINALLY:
        case cee_next: 
            break;
        case CEE_SWITCH:
            outBuffer = copySwitchInstruction(outBuffer, curBB);
            break;
        default:
            ASSERT(curBB->exitIL < cee_last);
            putOpcode(&outBuffer, curBB->exitIL);
            unsigned opSize = ILcodesSize[curBB->exitIL];
            unsigned addrSize = curBB->u.largeJump ? 4 : 1;
            int offset = computeJumpOffset(curBB, curBB->jumpDest, addrSize + opSize);
            if (curBB->u.largeJump) {
                ASSERT(offset != (char) (0xff & offset));
                SET_UNALIGNED_VAL32(outBuffer, offset);
                outBuffer += 4;
            } else {
                ASSERT(offset == (char) (0xff & offset));
                *(char*)outBuffer = (char) (0xff & offset);
                outBuffer ++;
            }
        }
    }

    return outBuffer;
}


// return the short ilcode form for a given jump instruction.
ILCODE ILGENREC::getShortOpcode(ILCODE longOpcode)
{

    switch (longOpcode) {

	case CEE_BEQ : longOpcode = CEE_BEQ_S; break;
	case CEE_BGE : longOpcode = CEE_BGE_S; break;
	case CEE_BGT : longOpcode = CEE_BGT_S; break;
	case CEE_BLE : longOpcode = CEE_BLE_S; break;
	case CEE_BLT : longOpcode = CEE_BLT_S; break;
	
	case CEE_BGE_UN : longOpcode = CEE_BGE_UN_S; break;
	case CEE_BGT_UN : longOpcode = CEE_BGT_UN_S; break;
	case CEE_BLE_UN : longOpcode = CEE_BLE_UN_S; break;
	case CEE_BLT_UN : longOpcode = CEE_BLT_UN_S; break;
	case CEE_BNE_UN : longOpcode = CEE_BNE_UN_S; break;

    case CEE_BRTRUE: longOpcode = CEE_BRTRUE_S; break;
    case CEE_BRFALSE: longOpcode = CEE_BRFALSE_S; break;
    case CEE_BR: longOpcode = CEE_BR_S; break;
    case CEE_LEAVE: longOpcode = CEE_LEAVE_S; break;
    default: ASSERT (!"bad jump opcode"); return CEE_ILLEGAL;
    }

    return longOpcode;
}

// mark a given block as visited, and reuturn true if this is the first time for that block
bool ILGENREC::markAsVisited(BBLOCK * block, void *data)
{
    if (block->reachable) return false;

    block->reachable = true;

    return true;
}

// mark all reachable blocks starting from start using the provided marking function
void ILGENREC::markAllReachableBB(BBLOCK * start, BBMarkFnc fnc, void * data)
{

AGAIN:
    bool visitNext = fnc(start, data);
    unsigned i;

    if (!visitNext) return;

    switch(start->exitIL) {
    case cee_stop:
    case CEE_ENDFINALLY:
    case CEE_RET: return;
    case cee_next:
        start = start->next;
        goto AGAIN;
    case CEE_LEAVE:
        if (start->v.gotoBlocked) {
            break;
        }
    case CEE_BR:
        start = start->jumpDest;
        goto AGAIN;
    case CEE_SWITCH:
        for (i = 0; i < start->switchDest->count; i++) {
            markAllReachableBB(start->switchDest->blocks[i].dest, fnc, data);
        }
        start = start->next;
        goto AGAIN;
    default:
        markAllReachableBB(start->jumpDest, fnc, data);
        start = start->next;
        goto AGAIN;
    }
}

ILCODE ILGENREC::getFlippedBranch(ILCODE br, bool reverseToUN)
{
    if (reverseToUN) {
        switch (br) {
        case CEE_BRFALSE: return CEE_BRTRUE;
        case CEE_BRTRUE: return CEE_BRFALSE;
        case CEE_BEQ:    return     CEE_BNE_UN;
        case CEE_BNE_UN:    return  CEE_BEQ;
        case CEE_BLT:    return     CEE_BGE_UN;
        case CEE_BLE:    return     CEE_BGT_UN;
        case CEE_BGT:    return     CEE_BLE_UN;
        case CEE_BGE:    return     CEE_BLT_UN;
        case CEE_BLT_UN:    return     CEE_BGE;
        case CEE_BLE_UN:    return     CEE_BGT;
        case CEE_BGT_UN:    return     CEE_BLE;
        case CEE_BGE_UN:    return     CEE_BLT;
        default:
            ASSERT(!"bad jump");
            return CEE_ILLEGAL;
        }
    } else {
        switch (br) {
        case CEE_BRFALSE: return CEE_BRTRUE;
        case CEE_BRTRUE: return CEE_BRFALSE;
        case CEE_BEQ:    return     CEE_BNE_UN;
        case CEE_BNE_UN:    return  CEE_BEQ;
        case CEE_BLT:    return     CEE_BGE;
        case CEE_BLE:    return     CEE_BGT;
        case CEE_BGT:    return     CEE_BLE;
        case CEE_BGE:    return     CEE_BLT;
        case CEE_BLT_UN:    return     CEE_BGE_UN;
        case CEE_BLE_UN:    return     CEE_BGT_UN;
        case CEE_BGT_UN:    return     CEE_BLE_UN;
        case CEE_BGE_UN:    return     CEE_BLT_UN;
        default:
            ASSERT(!"bad jump");
            return CEE_ILLEGAL;
        }
    }
}



#if DEBUG

unsigned ILGENREC::findBlockInList(BBLOCK **list, BBLOCK * block)
{
    for (unsigned i = 0; list[i] != block; i++);
    return i;
}

ILCODE ILGENREC::findInstruction(BYTE b1, BYTE b2)
{
    ILCODE il = (ILCODE) 0;
    do {
        if (ILcodes[il].b1 == b1 && ILcodes[il].b2 == b2) return il;
        il = (ILCODE) (il + 1);
    } while(true);
}

void ILGENREC::dumpAllBlocks() {

    if (!compiler()->IsRegString(L"*", L"BlockClass") && !compiler()->IsRegString(cls->name->text, L"BlockClass")) {
        return;
    }

    if (!compiler()->IsRegString(L"*", L"BlockMethod") && !compiler()->IsRegString(method->name->text, L"MethodClass")) {
        return;
    }

    wprintf(L"%s : %s\n", cls->name->text, method->name->text);


    unsigned count = 0;
    for (BBLOCK * current = firstBB; current; current = current->next) {
        count++;
    }

    BBLOCK ** list = (BBLOCK**) _alloca(sizeof(BBLOCK*) * count);

    unsigned i;
    for (i = 0, current = firstBB; current; current = current->next, i++) {
        list[i] = current;
    }

    for (i = 0, current = firstBB; current; current = current->next, i++) {
        bool tryEnd = false;
        bool handlerEnd = false;
        for (HANDLERINFO * hi = handlers; hi; hi = hi->next) {
            if (!hi->tryBegin) break;
            if (hi->tryBegin == current) {
                printf ("TRY: ");
            }
            if (hi->tryEnd == current) {
                tryEnd = true;
            }
            if (hi->handBegin == current) {
                printf ("HAND: ");
            }
            if (hi->handEnd == current) {
                handlerEnd = true;
            }
        }
        printf("Block %d : %p : ", findBlockInList(list, current), current);
        if (current->exitIL == cee_next && current->curLen == 0) {
            printf("EMPTY \n");
        } else {
            printf("Size(%d) : ", current->curLen);
            bool doJump = true;
            switch (current->exitIL) {
            case cee_next:
                printf("cee_next");
                doJump = false;
                break;
            case cee_stop:
                printf("cee_stop");
                doJump = false;
                break;
            case CEE_SWITCH:
            case CEE_RET:
                doJump = false;
            default:
                printf("%s ", ILnames[current->exitIL]);
            }
            if (doJump) {
                printf(" --> %d", findBlockInList(list, current->jumpDest));
            }
            if (tryEnd) {
                printf(" TRYEND");
            }
            if (handlerEnd) {
                printf(" HANDEND");
            }
            printf("\n");
        }
    }
}

void ILGENREC::dumpAllBlocksContent() {

    if (!compiler()->IsRegString(L"*", L"BlockClass") && !compiler()->IsRegString(cls->name->text, L"BlockClass")) {
        return;
    }

    if (!compiler()->IsRegString(L"*", L"BlockMethod") && !compiler()->IsRegString(method->name->text, L"BlockMethod")) {
        return;
    }

    wprintf(L"\n\n%s : %s\n", cls->name->text, method->name->text);


    unsigned count = 0;
    for (BBLOCK * current = firstBB; current; current = current->next) {
        count++;
    }

    BBLOCK ** list = (BBLOCK**) _alloca(sizeof(BBLOCK*) * count);

    unsigned i;
    for (i = 0, current = firstBB; current; current = current->next, i++) {
        list[i] = current;
    }

    DEBUGINFO * curDI = NULL;
    DWORD lastOff = (DWORD) -1;

    for (i = 0, current = firstBB; current; current = current->next, i++) {

        if (current->startOffset != (unsigned) -1 && current->startOffset != lastOff) {
            printf("[%d] ", current->startOffset);
            lastOff = current->startOffset;
        }

GETCDI:
        if (!curDI) {
            curDI = current->debugInfo;
            if (curDI) {
                while (curDI->prev) curDI = curDI->prev;
            }
        } else {
            if (curDI->endBlock == current) {
                if (curDI->endOffset == 0) {
                    curDI = NULL;
                    goto GETCDI;
                }
            }
        }

        printf("%d : ", i);
        BYTE *pb = current->code;
        size_t length = current->curLen;

        if (length > 0) {
            BYTE b = pb[0];
            BYTE b2;
            if (b == 0xfe) {
                b2 = pb[1];
            } else {
                b2 = b;
                b = 0xff;
            }
            ILCODE ilcode = findInstruction(b, b2);
            pb += ILcodesSize[ilcode];
            length -= ILcodesSize[ilcode];
            printf("%s\n", ILnames[ilcode]);
        }
        if (current->curLen) {
            if (curDI) {
                printf("[%d %d - %d %d]\n", curDI->begin.iLine +1 , curDI->begin.iChar +1 , curDI->end.iLine+1 , curDI->end.iChar+1);
                
                if (curDI->endBlock == current && curDI->endOffset == current->curLen) {
                    curDI = curDI->next;
                    if (!curDI) {
                        curDI = current->debugInfo;
                    }
                }
            }
        }

        if (current->exitIL == cee_next && current->curLen == 0) {
            if (current->sym && current->sym != (SYM *)0xCCCCCCCCCCCCCCCC) {
                printf("%ws", compiler()->ErrSym(current->sym));
            }
        } else {
            bool doJump = true;
            bool noToken = false;
            switch (current->exitIL) {
            case cee_next:
            case cee_stop:
            case CEE_RET:
                doJump = false;
                break;
            case CEE_SWITCH:
                doJump = false;
                noToken = true;
            default:
                printf("%s ", ILnames[current->exitIL]);
            }
            if (doJump) {
                printf(" --> %d ", findBlockInList(list, current->jumpDest));
                if (curDI) {
                    printf("[%d %d - %d %d]\n", curDI->begin.iLine +1 , curDI->begin.iChar +1 , curDI->end.iLine+1 , curDI->end.iChar+1);
                } else {
                    printf("\n");
                }
            } else {
                if (!noToken && !doJump && current->sym && current->sym != (SYM *)0xCCCCCCCCCCCCCCCC) {
                    printf("%ws\n", compiler()->ErrSym(current->sym));
                } else {
                    printf("\n");
                }
            }

        }

        if (curDI && curDI->endBlock == current) {
            curDI = NULL;
        }
    }

    // dump handlers
    for (HANDLERINFO * hi = handlers; hi; hi = hi->next) {
        wprintf(L"handler: start %d end %d hand %d last %d\n",
                findBlockInList(list, hi->tryBegin),
                findBlockInList(list, hi->tryEnd),
                findBlockInList(list, hi->handBegin),
                findBlockInList(list, hi->handEnd));
    }
}


#endif

void __fastcall ILGENREC::optimizeBranchesToNext()
{
    
    // branches to next block:
    for (BBLOCK * current = firstBB; current->next; current = current->next) {
        BBLOCK * target;
        switch(current->exitIL) {
        case CEE_RET:
        case cee_next:
        case CEE_ENDFINALLY:
        case CEE_SWITCH:
        case cee_stop:
        case CEE_LEAVE:
            break;
        default:
            target = current->next;
            do {
                if (current->jumpDest == target) {
                    if (current->exitIL != CEE_BR) {
                        //
                        // here we have a conditional branch with equal
                        // destination addresses
                        //
                        BYTE * offset = current->curLen + current->code;
                        // it's too late now to do any better :-( ...
                        putOpcode (&offset, CEE_POP);
                        if (current->exitIL != CEE_BRTRUE && current->exitIL != CEE_BRFALSE) {
                            putOpcode (&offset, CEE_POP);
                            current->curLen += ILcodesSize[CEE_POP];
                        }
                        current->curLen += ILcodesSize[CEE_POP];
                    }
                    current->exitIL = cee_next;
                    break;
                }
                if (target->isNOP()) {
                    target = target->next;
                } else {
                    break;
                }
            } while (true);
        }
    }

}

void __fastcall ILGENREC::optimizeBranchesOverBranches()
{

    BBLOCK *current;
    // cond branches over branches:
    // gotoif A, lab1             gotoif !a, lab2
    // goto lab2        --->      nop
    // lab1:                      lab1:
    for (current = firstBB; current->next; current = current->next) {
        BBLOCK * target;
        BBLOCK * next;
        switch(current->exitIL) {
        case CEE_RET:
        case cee_next:
        case cee_stop:
        case CEE_BR:
        case CEE_ENDFINALLY:
        case CEE_SWITCH:
        case CEE_LEAVE:
            break;
        default:
            next = current->next;
            while (next->isNOP()) next = next->next;
            if (next->exitIL == CEE_BR && !next->curLen) {
                target = next->next;
                if (target) {
                    while (target->isNOP()) target = target->next;
                    if (current->jumpDest == target) {
                        current->jumpDest = next->jumpDest;
                        // we wipe the branch:
                        next->exitIL = cee_next;
                        current->exitIL = getFlippedBranch(current->exitIL, current->v.condIsUN);
                    }
                }
            }
        }
    }
}

inline int AdvanceToNonNOP(BBLOCK **ppb) {
    int rval = 0;
    while((*ppb)->isNOP()) {
        if (ppb[0]->next->v.startsTry) {
            rval = -1;
        }
        *ppb = (*ppb)->next;
    }
    return rval;
}

void __fastcall ILGENREC::optimizeBranchesToNOPs()
{
   BBLOCK *current;
   unsigned i;
 
   for (current = firstBB; current->next; current = current->next) {
        switch(current->exitIL) {
        case CEE_RET:
        case cee_next:
        case cee_stop:
        case CEE_ENDFINALLY:
            break;
        case CEE_SWITCH:
            for (i = 0; i < current->switchDest->count; i++) {
                if (AdvanceToNonNOP(&current->switchDest->blocks[i].dest)) {
                    current->switchDest->blocks[i].jumpIntoTry = true;
                }
            }
            break;
        default: 
            if (AdvanceToNonNOP(&current->jumpDest)) {
                current->v.jumpIntoTry = true;
            }
        }
   }

   // must advance handlers past NOPs as well
   // so that handlers don't point to bogusly unreachable blocks
   for (HANDLERINFO * hi = handlers; hi; hi = hi->next) {
      AdvanceToNonNOP(&hi->tryBegin);
      AdvanceToNonNOP(&hi->tryEnd);
      AdvanceToNonNOP(&hi->handBegin);
      AdvanceToNonNOP(&hi->handEnd);
   }
}

// optimize branches

// This name is historical.  Please do not change it.
void ILGENREC::suckUpGotos()
{

#if DEBUG
    if (compiler()->GetRegDWORD("Before")) {
        dumpAllBlocksContent();
    }
#endif

    BBLOCK * current;
    unsigned i;

    dumpAllBlocks();
    optimizeBranchesToNOPs();
    dumpAllBlocks();

    // INVARIANT:  all br instructions have actual targets that cannot be skipped.

    if (compiler()->options.m_fOPTIMIZATIONS) {
        // branches to branches and branches to ret
        for (current = firstBB; current->next; current = current->next) {
            switch(current->exitIL) {
            case CEE_RET:
            case cee_next:
            case cee_stop:
            case CEE_ENDFINALLY:
                break;
            case CEE_SWITCH:
                // Need to examine if any of the cases go to a br, in which case they canbe
                // redirected furher...
                for (i = 0; i < current->switchDest->count; i++) {
AGAINSW:
                    BBLOCK * targetBlock = current->switchDest->blocks[i].dest;
                    if (!targetBlock->curLen) {
                        if (&(current->switchDest->blocks[i]) == targetBlock->markedWithSwitch) {
                            // protect against cycles > 1 in size
                            targetBlock->jumpDest = targetBlock;
                            continue;
                        }
                        if (targetBlock->exitIL == CEE_BR && targetBlock->jumpDest != targetBlock) {
                            if (!current->switchDest->blocks[i].jumpIntoTry) {
                                targetBlock->markedWithSwitch = &(current->switchDest->blocks[i]);
                                current->switchDest->blocks[i].jumpIntoTry = targetBlock->v.jumpIntoTry;
                                current->switchDest->blocks[i].dest = targetBlock->jumpDest;
                                goto AGAINSW;
                            }
                        }
                    }
                }
                break;
            default:
                // We need to catch extended cycles.  we do this by writing current into the code field of the
                // destination if we suck it in.  since we only suck in empty blocks we don't overwrite any code
AGAINBR:
                if (!current->jumpDest->curLen) {
                    if (current == current->jumpDest->markedWith) {
                        // we sucked this in already... which means that we hit a cycle, so we might as well
                        // emit a jump to ourselves...
                        current->jumpDest->jumpDest = current->jumpDest;
                        break;
                    }
                    if (current->jumpDest->exitIL == CEE_BR && current->jumpDest->jumpDest != current->jumpDest) {
                        if (!current->v.jumpIntoTry) {
                            if (current->jumpDest->v.jumpIntoTry) {
                                current->v.jumpIntoTry = true;
                            }
                            //  we suck in the destination, and mark it as sucked in
                            current->jumpDest->markedWith = current;
                            current->jumpDest = current->jumpDest->jumpDest;
                            goto AGAINBR;
                        }
                    }
                }
                if (current->exitIL == CEE_BR && current->jumpDest->exitIL == CEE_RET && current->jumpDest->curLen == ILcodesSize[CEE_RET]) {
                    current->exitIL = CEE_RET;
                    BYTE * offset = current->curLen + current->code;
                    putOpcode (&offset, CEE_RET);
                    current->curLen += ILcodesSize[CEE_RET];
                }
            }
        }

        // INVARIANT: No br instruction has br as its immediate target
        //dumpAllBlocks();

        optimizeBranchesToNext();
        dumpAllBlocks();
    }

    // INVARIANT: No br instruction has an offset of 0.

    for (current = firstBB; current; current = current->next) {
        current->reachable = false;
    }

    // mark all normally reachable blocks:
    markAllReachableBB(firstBB, ILGENREC::markAsVisited, NULL);

    // now, mark all blocks reachable from reachable exception handlers...

    unsigned unreachCountOld = ehCount;

EXCEPAGAIN:

    unsigned unreachable = 0;

    HANDLERINFO *hi;
    for (hi = handlers; hi; hi = hi->next) {
        bool reached = false;
        for (current = hi->tryBegin; current && current != hi->tryEnd->next; current = current->next) {
            if (current->reachable) {
                reached = true;
                if (!hi->handBegin->reachable) {        
                    markAllReachableBB(hi->handBegin, ILGENREC::markAsVisited, NULL);
                }
                break;
            }
        }
        if (!reached) {
            unreachable++;
        }
    }

    if (unreachable && (unreachable != unreachCountOld)) {
        unreachCountOld = unreachable;
        goto EXCEPAGAIN;
    }

    ASSERT(unreachable <= ehCount);


    bool redoTryFinally;
    
REDOTRYFINALLY:
    redoTryFinally = false;

    // now, remove handlers for empty bodies if optimizing
    if (compiler()->options.m_fOPTIMIZATIONS) {

        bool changed = false;
        
DOHANDLERS:

        if (changed) {
            redoTryFinally = true;

            changed = false;
        }

        for (hi = handlers; hi; hi = hi->next) {

            if (!hi->tryBegin) continue;

            BBLOCK * start = hi->tryBegin;
            while(start != hi->tryEnd && (start->isNOP() || !start->reachable)) {
                start = start->next;
            }
            if (start == hi->tryEnd) {

                changed = true;

                // ok, this try block is empty, so we have one of 2 cases, if we optimize:

                // case one: its a catch, so we transform the catch into a nop...
        
                if (hi->type) {
                    // if it's a catch, then it will never be executed, so
                    // it's sufficient if we skip it entirely...
                    start = hi->handBegin;
                    while (start != hi->handEnd->next) {
                        start->reachable = false;
                        start = start->next;
                    }

                    // also, eliminate the CEE_LEAVE at the end of the try block, as
                    // it is the same as a fallthrough now (since there is no catch for it to
                    // jump over...
                    ASSERT(hi->tryEnd->exitIL == CEE_LEAVE || hi->tryEnd->exitIL == CEE_BR);
                    hi->tryEnd->exitIL = CEE_BR;


                    hi->tryBegin = NULL;

                // case 2: its a finally:
                } else {
                    // if it's a finally, then we convert it to a normal block, by
                    // removing the END_FINALLY instruction...
                    ASSERT(start->exitIL == CEE_LEAVE);
                    start->exitIL = cee_next;
                    start->curLen = 0;

                    // remove the CEE_ENDFINALLY instruction:
                    hi->handEnd->reachable = false;

                    // in both cases, we need to make sure that we don't emit EIT information
                    // for this try:
                    hi->tryBegin = NULL;
                }
            } else {
                // non empty try:

                // the trybody had code in it, buf if the finally body has no code
                // we can optimize it away...
                if (!hi->type) {
                    BBLOCK * start = hi->handBegin;
                    while (start != hi->handEnd && (start->isNOP() || !start->reachable)) {
                        start = start->next;
                    }
                    if (start == hi->handEnd) {
                        // also, remove the CEE_LEAVE as well as the CEE_ENDFINALLY instrs
                        hi->tryEnd->reachable = false;
                        hi->handEnd->reachable = false;
                
                        // no code, in finally block, so just mark it as unreachable and we
                        // will not emit EIT info for it
                        hi->tryBegin = NULL;

                        changed = true;

                    }
                }
            }
        }
        dumpAllBlocks();

        if (changed) goto DOHANDLERS;

    } else {

        // if not optimizing, we will merely remove unreachable trys


        for (hi = handlers; hi; hi = hi->next) {

            if (!hi->handBegin->reachable) {
                hi->tryBegin = NULL; // this effectively removes the try...
            }
        }
    }

    // we need to insure that those leave's don't go into outer space...
    for (current = firstBB; current && blockedLeave; current = current->next) {
        if (current->v.gotoBlocked) {
            blockedLeave--;
            if (current->reachable) {
                BBLOCK * temp = current->jumpDest;
                while (temp && !temp->v.startsCatchOrFinally && !temp->v.endsFinally && (!temp->reachable || (!temp->curLen && (temp->exitIL == cee_next || temp->exitIL == cee_stop)))) {
                    temp = temp->next;
                }
                if (!temp || temp->v.startsCatchOrFinally || temp->v.endsFinally) {
                    // ok, this leave either points to lala land, or into a catch or finally (also illegal),
                    // or, crosses a finally boundary (some false positives here, buts that's ok)
                    // so we need to make it point to some sane piece of code, namely an infinite loop...

                    current->jumpDest->exitIL = CEE_BR;
                    current->jumpDest->curLen = 0;
                    current->jumpDest->jumpDest = current->jumpDest;
                    current->jumpDest->reachable = true;
					current->jumpDest->debugInfo = NULL;
                }
            }
        }
    }

    // now, suck up all unreachable blocks from this list...
    // [Well, actually, all we do is wipe the code and the exit instruction...]
    for (current = firstBB; current; current = current->next) {
        if (!current->reachable) {
            current->curLen = 0;
            current->exitIL = cee_next;
            current->debugInfo = NULL;
        }
    }

    // if this were unreachable, it could have been wiped, so let's reset it just in case
    if (currentBB->exitIL == cee_next) {
        currentBB->exitIL = cee_stop;
    }
    dumpAllBlocks();

    if (compiler()->options.m_fOPTIMIZATIONS) {
        // now that we no longer have unreachable code, optimize branches over branches...
        optimizeBranchesOverBranches();
        //dumpAllBlocks();

        // Now, we might have enabled more branches to next which used to be
        // branches over dead code... so let's do that

        optimizeBranchesToNext();

        dumpAllBlocks();
        // at this point there are no other opts to perform...
        // since branchesToNext removes branches, there are no new cases of
        // branchesToBranches or branchesToRet which can be optimized...

        if (redoTryFinally) goto REDOTRYFINALLY;

    }


}

// array of instruction sizes:
const BYTE ILGENREC::ILcodesSize [] = {
#define OPDEF(id, name, pop, push, operand, type, len, b1, b2, cf) len ,
#include "opcode.def"
#undef OPDEF
};

#if DEBUG
const LPSTR ILGENREC::ILnames[] = {
#define OPDEF(id, name, pop, push, operand, type, len, b1, b2, cf) name,
#include "opcode.def"
#undef OPDEF
};
#endif

const ILCODE ILGENREC::ILaddrLoad [2][2] =
{
    { CEE_LDLOCA_S, CEE_LDLOCA, },
    { CEE_LDARGA_S, CEE_LDARGA, },
};

// array of load store instructions.  if in inline verion is not present
// we indicate this w/ CEE_ILLEGA which means that we have to use the
// _S version
const ILCODE ILGENREC::ILlsTiny [4][6] =
{
    { CEE_LDLOC_0, CEE_LDLOC_1, CEE_LDLOC_2, CEE_LDLOC_3, CEE_LDLOC_S, CEE_LDLOC, },
    { CEE_STLOC_0, CEE_STLOC_1, CEE_STLOC_2, CEE_STLOC_3, CEE_STLOC_S, CEE_STLOC, },
    { CEE_LDARG_0, CEE_LDARG_1, CEE_LDARG_2, CEE_LDARG_3, CEE_LDARG_S, CEE_LDARG, },
    { CEE_ILLEGAL, CEE_ILLEGAL, CEE_ILLEGAL, CEE_ILLEGAL, CEE_STARG_S, CEE_STARG, },
};


// array of compare/jump instructions.
const ILCODE ILGENREC::ILcjInstrUN[6][6] =
{
    { CEE_ILLEGAL,  CEE_ILLEGAL,    CEE_NEG,        CEE_ILLEGAL,    CEE_ILLEGAL, CEE_ILLEGAL, }, // ==
    { CEE_NEG,      CEE_ILLEGAL,    CEE_ILLEGAL,    CEE_ILLEGAL,    CEE_ILLEGAL, CEE_ILLEGAL, }, // !=
    { CEE_ILLEGAL,  CEE_ILLEGAL,    CEE_NEG,        CEE_CLT_UN,     CEE_BLT_UN,  CEE_BGE_UN,  }, // <
    { CEE_NEG,      CEE_CGT_UN,     CEE_ILLEGAL,    CEE_ILLEGAL,    CEE_BLE_UN,  CEE_BGT_UN,  }, // <=
    { CEE_ILLEGAL,  CEE_ILLEGAL,    CEE_NEG,        CEE_CGT_UN,     CEE_BGT_UN,  CEE_BLE_UN,  }, // >
    { CEE_NEG,      CEE_CLT_UN,     CEE_ILLEGAL,    CEE_ILLEGAL,    CEE_BGE_UN,  CEE_BLT_UN,  }, // >=
};

// array of compare/jump instructions for UN extensions.
const ILCODE ILGENREC::ILcjInstr[6][6] =
{
    { CEE_ILLEGAL,  CEE_CEQ,    CEE_NEG,        CEE_CEQ,    CEE_BEQ,    CEE_BNE_UN, }, // ==
    { CEE_NEG,      CEE_CEQ,    CEE_ILLEGAL,    CEE_CEQ,    CEE_BNE_UN, CEE_BEQ,    }, // !=
    { CEE_ILLEGAL,  CEE_CLT,    CEE_NEG,        CEE_CLT,    CEE_BLT,    CEE_BGE,    }, // <
    { CEE_NEG,      CEE_CGT,    CEE_ILLEGAL,    CEE_CGT,    CEE_BLE,    CEE_BGT,    }, // <=
    { CEE_ILLEGAL,  CEE_CGT,    CEE_NEG,        CEE_CGT,    CEE_BGT,    CEE_BLE,    }, // >
    { CEE_NEG,      CEE_CLT,    CEE_ILLEGAL,    CEE_CLT,    CEE_BGE,    CEE_BLT,    }, // >=
};

const ILCODE ILGENREC::ILarithInstr[EK_ARRLEN - EK_ADD + 1] = {
    CEE_ADD,    // EK_ADD
    CEE_SUB,    // EK_SUB
    CEE_MUL,    // EK_MUL
    CEE_DIV,    // EK_DIV
    CEE_REM,    // EK_MOD
    
    CEE_AND,    // EK_BITAND
    CEE_OR,     // EK_BITOR
    CEE_XOR,    // EK_BITXOR
    CEE_SHL,    // EK_LSHIFT
    CEE_SHR,    // EK_RSHIFT
    CEE_NEG,    // EK_NEG
    CEE_ILLEGAL,// EK_UPLUS
    CEE_NOT,    // EK_BITNOT
    CEE_LDLEN,  // EK_ARRLEN
};

const ILCODE ILGENREC::ILarithInstrUN[EK_ARRLEN - EK_ADD + 1] = {
    CEE_ADD,    // EK_ADD
    CEE_SUB,    // EK_SUB
    CEE_MUL,    // EK_MUL
    CEE_DIV_UN, // EK_DIV
    CEE_REM_UN, // EK_MOD
    
    CEE_AND,    // EK_BITAND
    CEE_OR,     // EK_BITOR
    CEE_XOR,    // EK_BITXOR
    CEE_SHL,    // EK_LSHIFT
    CEE_SHR_UN, // EK_RSHIFT
    CEE_NEG,    // EK_NEG
    CEE_ILLEGAL,// EK_UPLUS
    CEE_NOT,    // EK_BITNOT
    CEE_LDLEN,  // EK_ARRLEN
};

const ILCODE ILGENREC::ILarithInstrOvf[EK_ARRLEN - EK_ADD + 1] = {
    CEE_ADD_OVF,// EK_ADD
    CEE_SUB_OVF,// EK_SUB
    CEE_MUL_OVF,// EK_MUL
    CEE_DIV,// EK_DIV
    CEE_REM,    // EK_MOD
    
    CEE_AND,    // EK_BITAND
    CEE_OR,     // EK_BITOR
    CEE_XOR,    // EK_BITXOR
    CEE_SHL,    // EK_LSHIFT
    CEE_SHR,    // EK_RSHIFT
    CEE_NEG,    // EK_NEG
    CEE_ILLEGAL,// EK_UPLUS
    CEE_NOT,    // EK_BITNOT
    CEE_LDLEN,  // EK_ARRLEN
};

const ILCODE ILGENREC::ILarithInstrUNOvf[EK_ARRLEN - EK_ADD + 1] = {
    CEE_ADD_OVF_UN,// EK_ADD
    CEE_SUB_OVF_UN,// EK_SUB
    CEE_MUL_OVF_UN,// EK_MUL
    CEE_DIV_UN, // EK_DIV
    CEE_REM_UN, // EK_MOD
    
    CEE_AND,    // EK_BITAND
    CEE_OR,     // EK_BITOR
    CEE_XOR,    // EK_BITXOR
    CEE_SHL,    // EK_LSHIFT
    CEE_SHR_UN, // EK_RSHIFT
    CEE_NEG,    // EK_NEG
    CEE_ILLEGAL,// EK_UPLUS
    CEE_NOT,    // EK_BITNOT
    CEE_LDLEN,  // EK_ARRLEN
};

const ILCODE ILGENREC::ILstackLoad[FT_COUNT] = {
CEE_ILLEGAL,   //    FT_NONE,        // No fundemental type
CEE_LDIND_I1, //    FT_I1,
CEE_LDIND_I2, //    FT_I2,
CEE_LDIND_I4, //    FT_I4,
CEE_LDIND_U1, //    FT_U1,
CEE_LDIND_U2, //    FT_U2,
CEE_LDIND_U4, //    FT_U4,
CEE_LDIND_I8, //    FT_I8,
CEE_LDIND_I8, //    FT_U8,          // integral types
CEE_LDIND_R4, //    FT_R4,
CEE_LDIND_R8, //    FT_R8,          // floating types
CEE_LDIND_REF,//    FT_REF,         // reference type
CEE_LDOBJ  , //    FT_STRUCT,      // structure type
CEE_LDIND_I,
};


const ILCODE ILGENREC::ILstackStore[FT_COUNT] = {
CEE_ILLEGAL,   //    FT_NONE,        // No fundemental type
CEE_STIND_I1, //    FT_I1,
CEE_STIND_I2, //    FT_I2,
CEE_STIND_I4, //    FT_I4,
CEE_STIND_I1, //    FT_U1,
CEE_STIND_I2, //    FT_U2,
CEE_STIND_I4, //    FT_U4,
CEE_STIND_I8, //    FT_I8,
CEE_STIND_I8, //    FT_U8,          // integral types
CEE_STIND_R4, //    FT_R4,
CEE_STIND_R8, //    FT_R8,          // floating types
CEE_STIND_REF,//    FT_REF,         // reference type
CEE_STOBJ  , //    FT_STRUCT,      // structure type
CEE_STIND_I,
};

const ILCODE ILGENREC::ILarrayLoad[FT_COUNT] = {
CEE_ILLEGAL,   //    FT_NONE,        // No fundemental type
CEE_LDELEM_I1, //    FT_I1,
CEE_LDELEM_I2, //    FT_I2,
CEE_LDELEM_I4, //    FT_I4,
CEE_LDELEM_U1, //    FT_U1,
CEE_LDELEM_U2, //    FT_U2,
CEE_LDELEM_U4, //    FT_U4,
CEE_LDELEM_I8, //    FT_I8,
CEE_LDELEM_I8, //    FT_U8,          // integral types
CEE_LDELEM_R4, //    FT_R4,
CEE_LDELEM_R8, //    FT_R8,          // floating types
CEE_LDELEM_REF,//    FT_REF,         // reference type
CEE_ILLEGAL  , //    FT_STRUCT,      // structure type
CEE_LDELEM_I,
};

const ILCODE ILGENREC::ILarrayStore[FT_COUNT] = {
CEE_ILLEGAL,   //    FT_NONE,        // No fundemental type
CEE_STELEM_I1, //    FT_I1,
CEE_STELEM_I2, //    FT_I2,
CEE_STELEM_I4, //    FT_I4,
CEE_STELEM_I1, //    FT_U1,
CEE_STELEM_I2, //    FT_U2,
CEE_STELEM_I4, //    FT_U4,
CEE_STELEM_I8, //    FT_I8,
CEE_STELEM_I8, //    FT_U8,          // integral types
CEE_STELEM_R4, //    FT_R4,
CEE_STELEM_R8, //    FT_R8,          // floating types
CEE_STELEM_REF,//    FT_REF,         // reference type
CEE_ILLEGAL  , //    FT_STRUCT,      // structure type
CEE_STELEM_I,
};


#define NOP cee_next
#define U1 CEE_CONV_U1
#define U2 CEE_CONV_U2
#define U4 CEE_CONV_U4
#define U8 CEE_CONV_U8
#define I1 CEE_CONV_I1
#define I2 CEE_CONV_I2
#define I4 CEE_CONV_I4
#define I8 CEE_CONV_I8
#define R4 CEE_CONV_R4
#define R8 CEE_CONV_R8
#define UR CEE_CONV_R_UN
#define ILL CEE_ILLEGAL

const ILCODE ILGENREC::simpleTypeConversions[NUM_SIMPLE_TYPES][NUM_SIMPLE_TYPES] = {
//        to: BYTE    I2      I4      I8      FLT     DBL     DEC     CHAR    BOOL     SBYTE     U2     U4     U8
/* from */
/* BYTE */ {  NOP    ,NOP    ,NOP    ,U8     ,R4     ,R8     ,ILL    ,U2     ,ILL     ,I1       ,NOP   ,NOP   ,U8},
/*   I2 */ {  U1     ,NOP    ,NOP    ,I8     ,R4     ,R8     ,ILL    ,U2     ,ILL     ,I1       ,U2    ,U4    ,I8},
/*   I4 */ {  U1     ,I2     ,NOP    ,I8     ,R4     ,R8     ,ILL    ,U2     ,ILL     ,I1       ,U2    ,NOP   ,I8},
/*   I8 */ {  U1     ,I2     ,I4     ,NOP    ,R4     ,R8     ,ILL    ,U2     ,ILL     ,I1       ,U2    ,U4    ,NOP},
/*  FLT */ {  U1     ,I2     ,I4     ,I8     ,R4     ,R8     ,ILL    ,U2     ,ILL     ,I1       ,U2    ,U4    ,U8},
/*  DBL */ {  U1     ,I2     ,I4     ,I8     ,R4     ,R8     ,ILL    ,U2     ,ILL     ,I1       ,U2    ,U4    ,U8},
/*  DEC */ {  ILL    ,ILL    ,ILL    ,ILL    ,R4     ,R8     ,ILL    ,U2     ,ILL     ,ILL      ,ILL   ,ILL   ,ILL},
/* CHAR */ {  U1     ,I2     ,NOP    ,I8     ,R4     ,R8     ,ILL    ,NOP    ,ILL     ,I1       ,NOP   ,NOP   ,U8},
/* BOOL */ {  ILL    ,ILL    ,ILL    ,ILL    ,ILL    ,ILL    ,ILL    ,ILL    ,ILL     ,ILL      ,ILL   ,ILL   ,ILL},
/*SBYTE */ {  U1     ,I2     ,I4     ,I8     ,R4     ,R8     ,ILL    ,U2     ,ILL     ,NOP      ,U2    ,U4    ,I8},
/*   U2 */ {  U1     ,I2     ,I4     ,U8     ,R4     ,R8     ,ILL    ,NOP    ,ILL     ,I1       ,NOP   ,NOP   ,U8},
/*   U4 */ {  U1     ,I2     ,NOP    ,U8     ,UR     ,UR     ,ILL    ,U2     ,ILL     ,I1       ,U2    ,NOP   ,U8},
/*   U8 */ {  U1     ,I2     ,I4     ,NOP    ,UR     ,UR     ,ILL    ,U2     ,ILL     ,I1       ,U2    ,U4    ,NOP}
};

const ILCODE ILGENREC::simpleTypeConversionsEx[NUM_SIMPLE_TYPES][NUM_SIMPLE_TYPES] = {
//        to: BYTE    I2      I4      I8      FLT     DBL     DEC     CHAR    BOOL     SBYTE     U2     U4     U8
/* from */
/* BYTE */ {  ILL    ,ILL    ,ILL    ,ILL    ,ILL    ,ILL    ,ILL    ,ILL    ,ILL     ,ILL      ,ILL   ,ILL   ,ILL},
/*   I2 */ {  ILL    ,ILL    ,ILL    ,ILL    ,ILL    ,ILL    ,ILL    ,ILL    ,ILL     ,ILL      ,ILL   ,ILL   ,ILL},
/*   I4 */ {  ILL    ,ILL    ,ILL    ,ILL    ,ILL    ,ILL    ,ILL    ,ILL    ,ILL     ,ILL      ,ILL   ,ILL   ,ILL},
/*   I8 */ {  ILL    ,ILL    ,ILL    ,ILL    ,ILL    ,ILL    ,ILL    ,ILL    ,ILL     ,ILL      ,ILL   ,ILL   ,ILL},
/*  FLT */ {  ILL    ,ILL    ,ILL    ,ILL    ,ILL    ,ILL    ,ILL    ,ILL    ,ILL     ,ILL      ,ILL   ,ILL   ,ILL},
/*  DBL */ {  ILL    ,ILL    ,ILL    ,ILL    ,ILL    ,ILL    ,ILL    ,ILL    ,ILL     ,ILL      ,ILL   ,ILL   ,ILL},
/*  DEC */ {  ILL    ,ILL    ,ILL    ,ILL    ,ILL    ,ILL    ,ILL    ,ILL    ,ILL     ,ILL      ,ILL   ,ILL   ,ILL},
/* CHAR */ {  ILL    ,ILL    ,ILL    ,ILL    ,ILL    ,ILL    ,ILL    ,ILL    ,ILL     ,ILL      ,ILL   ,ILL   ,ILL},
/* BOOL */ {  ILL    ,ILL    ,ILL    ,ILL    ,ILL    ,ILL    ,ILL    ,ILL    ,ILL     ,ILL      ,ILL   ,ILL   ,ILL},
/*SBYTE */ {  ILL    ,ILL    ,ILL    ,ILL    ,ILL    ,ILL    ,ILL    ,ILL    ,ILL     ,ILL      ,ILL   ,ILL   ,ILL},
/*   U2 */ {  ILL    ,ILL    ,ILL    ,ILL    ,ILL    ,ILL    ,ILL    ,ILL    ,ILL     ,ILL      ,ILL   ,ILL   ,ILL},
/*   U4 */ {  ILL    ,ILL    ,ILL    ,ILL    ,R4     ,R8     ,ILL    ,ILL    ,ILL     ,ILL      ,ILL   ,ILL   ,ILL},
/*   U8 */ {  ILL    ,ILL    ,ILL    ,ILL    ,R4     ,R8     ,ILL    ,ILL    ,ILL     ,ILL      ,ILL   ,ILL   ,ILL}
};

#undef NOP
#undef U1
#undef U2
#undef U4
#undef U8
#undef I1
#undef I2
#undef I4
#undef I8
#undef R4
#undef R8
#undef UR
#undef ILL

#define NOP cee_next
#define U1  CEE_CONV_OVF_U1 // signed to U?
#define U2  CEE_CONV_OVF_U2
#define U4  CEE_CONV_OVF_U4
#define U8  CEE_CONV_OVF_U8
#define U1U CEE_CONV_OVF_U1_UN // unsigned to U?
#define U2U CEE_CONV_OVF_U2_UN
#define U4U CEE_CONV_OVF_U4_UN
#define U8U CEE_CONV_OVF_U8_UN
#define I1  CEE_CONV_OVF_I1 // signed to I?
#define I2  CEE_CONV_OVF_I2
#define I4  CEE_CONV_OVF_I4
#define I8  CEE_CONV_OVF_I8
#define I1U CEE_CONV_OVF_I1_UN // unsigned to I?
#define I2U CEE_CONV_OVF_I2_UN
#define I4U CEE_CONV_OVF_I4_UN
#define I8U CEE_CONV_OVF_I8_UN
#define U8N CEE_CONV_U8   // no overflow check
#define I8N CEE_CONV_I8     // no overflow check
#define R4 CEE_CONV_R4
#define R8 CEE_CONV_R8
#define UR CEE_CONV_R_UN
#define ILL CEE_ILLEGAL

const ILCODE ILGENREC::simpleTypeConversionsOvf[NUM_SIMPLE_TYPES][NUM_SIMPLE_TYPES] = {
//        to: BYTE    I2      I4      I8      FLT     DBL     DEC     CHAR    BOOL     SBYTE     U2     U4     U8
/* from */
/* BYTE */ {  NOP    ,NOP    ,NOP    ,I8U    ,R4     ,R8     ,ILL    ,NOP    ,ILL     ,I1U      ,NOP   ,NOP   ,U8N},
/*   I2 */ {  U1     ,NOP    ,NOP    ,I8N    ,R4     ,R8     ,ILL    ,U2     ,ILL     ,I1       ,U2    ,U4    ,U8},
/*   I4 */ {  U1     ,I2     ,NOP    ,I8N    ,R4     ,R8     ,ILL    ,U2     ,ILL     ,I1       ,U2    ,U4    ,U8},
/*   I8 */ {  U1     ,I2     ,I4     ,NOP    ,R4     ,R8     ,ILL    ,U2     ,ILL     ,I1       ,U2    ,U4    ,U8},
/*  FLT */ {  U1     ,I2     ,I4     ,I8     ,R4     ,R8     ,ILL    ,U2     ,ILL     ,I1       ,U2    ,U4    ,U8},
/*  DBL */ {  U1     ,I2     ,I4     ,I8     ,R4     ,R8     ,ILL    ,U2     ,ILL     ,I1       ,U2    ,U4    ,U8},
/*  DEC */ {  ILL    ,ILL    ,ILL    ,ILL    ,R4     ,R8     ,ILL    ,U2     ,ILL     ,ILL      ,ILL   ,ILL   ,ILL},
/* CHAR */ {  U1U    ,I2U    ,NOP    ,I8U    ,R4     ,R8     ,ILL    ,NOP    ,ILL     ,I1U      ,NOP   ,NOP   ,U8U},
/* BOOL */ {  ILL    ,ILL    ,ILL    ,ILL    ,ILL    ,ILL    ,ILL    ,ILL    ,ILL     ,ILL      ,ILL   ,ILL   ,ILL},
/*SBYTE */ {  U1     ,NOP    ,I4     ,I8N    ,R4     ,R8     ,ILL    ,U2     ,ILL     ,NOP      ,U2    ,U4    ,U8},
/*   U2 */ {  U1U    ,I2U    ,NOP    ,I8U    ,R4     ,R8     ,ILL    ,NOP    ,ILL     ,I1U      ,NOP   ,NOP   ,U8N},
/*   U4 */ {  U1U    ,I2U    ,I4U    ,I8U    ,UR     ,UR     ,ILL    ,U2U    ,ILL     ,I1U      ,U2U   ,NOP   ,U8N},
/*   U8 */ {  U1U    ,I2U    ,I4U    ,I8U    ,UR     ,UR     ,ILL    ,U2U    ,ILL     ,I1U      ,U2U   ,U4U   ,NOP}
};

#undef NOP
#undef U1
#undef U2
#undef U4
#undef U8
#undef U1U
#undef U2U
#undef U4U
#undef U8U
#undef I1
#undef I2
#undef I4
#undef I8
#undef I1U
#undef I2U
#undef I4U
#undef I8U
#undef R4
#undef R8
#undef UR
#undef ILL


// stack behaviour of operations
const int ILGENREC::ILStackOps [] = {
#define Pop0 0
#define Pop1 1
#define PopI 1
#define PopR4 1
#define PopR8 1
#define PopI8 1
#define PopRef 1
#define VarPop 0
#define Push0 0
#define Push1 1
#define PushI 1
#define PushR4 1
#define PushR8 1
#define PushI8 1
#define PushRef 1
#define VarPush 0
#define OPDEF(id, name, pop, push, operand, type, len, b1, b2, cf) (push) - (pop) ,
#include "opcode.def"
#undef Pop0
#undef Pop1
#undef PopI
#undef PopR4
#undef PopR8
#undef PopI8
#undef PopRef
#undef VarPop
#undef Push0
#undef Push1
#undef PushI
#undef PushR4
#undef PushR8
#undef PushI8
#undef PushRef
#undef VarPush
#undef OPDEF
};
