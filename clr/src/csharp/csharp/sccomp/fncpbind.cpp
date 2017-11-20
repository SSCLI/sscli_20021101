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
// File: fncpbind.cpp
//
// Routines for analyzing the bound body of a function
// ===========================================================================

#include "stdafx.h"

#include "compiler.h"


#define BIT_SHIFT		5   // Shift by this much
#define BIT_MASK		31	// This is how many bits can go in a 'slot'
#define MAX_SIZE		32  // This is the size of a 'slot'


BITSET * __fastcall BITSET::setBit(unsigned bit)
{
    if (((DWORD_PTR)this) & 1) {
        ASSERT(bit <= BIT_MASK && bit >= 1);
        DWORD_PTR bits = (DWORD_PTR) this;
        bits |= (((DWORD_PTR)1) << bit);
        return (BITSET*) bits;
    }
    return setBitLarge(bit);
}

BITSET * __fastcall BITSET::setBitLarge(unsigned bit)
{
    BITSETIMP * bitset = (BITSETIMP*)this;
    bitset->bitArray[bit >> BIT_SHIFT] |= ((DWORD_PTR)1) << (bit & BIT_MASK);
    return this;
}

// returns the bits between start(inclusive) and start+size (exclusive)
// set to 1, all other bits to 0
DWORD_PTR __forceinline GetMask(unsigned start, unsigned size)
{
    ASSERT(start <= BIT_MASK && (start + size <= MAX_SIZE));

    return (((DWORD_PTR) -1) << (MAX_SIZE - size)) >> (MAX_SIZE - size - start);
}

bool __fastcall BITSET::isEqual(BITSET * other)
{
    if (!this) return !other;
    if (((DWORD_PTR)this) & 1) {
        return ((DWORD_PTR)this) == ((DWORD_PTR)other);
    }
    return isEqualLarge(other); 
}

bool __fastcall BITSET::isEqualLarge(BITSET * other)
{
    BITSETIMP * bitset = (BITSETIMP*)this;
    BITSETIMP * compare = (BITSETIMP*)other; 

    if (!compare) return false;

    size_t size = bitset->arrayEnd - bitset->bitArray;
    return !memcmp(bitset->bitArray, compare->bitArray, size * sizeof(DWORD_PTR));
}

BITSET * __fastcall BITSET::setBits(unsigned start, unsigned size)
{
    if (((DWORD_PTR)this) & 1) {
        DWORD_PTR bits = (DWORD_PTR) this;
        bits |= GetMask(start, size);
        return (BITSET *) bits;
    } 
    return setBitsLarge(start, size);
}

BITSET * __fastcall BITSET::setBitsLarge(unsigned start, unsigned size)
{

    if (size == 1) return setBitLarge(start);

    BITSETIMP * bitset = (BITSETIMP*)this;
    
    unsigned index = start >> BIT_SHIFT;
    start = start & BIT_MASK;
    unsigned thisSize;

    while ((thisSize = min(size, MAX_SIZE - start))) {
        bitset->bitArray[index] |= GetMask(start, thisSize);
        if (size > thisSize)
            size -= thisSize;
        else
            size = 0;
        start = 0;
        index ++;
    }

    return this;

}

DWORD_PTR __fastcall BITSET::testBit(unsigned bit)
{
    if (((DWORD_PTR)this) & 1) {
        ASSERT(bit <= BIT_MASK && bit >= 1);
        DWORD_PTR bits = (DWORD_PTR) this;
        return (bits & (((DWORD_PTR)1) << bit));
    }
    return testBitLarge(bit);
}

bool __fastcall BITSET::isZero()
{
    if (((DWORD_PTR)this) & 1) {
        DWORD_PTR bits = (DWORD_PTR) this;
        return bits == 1;
    }
    return isZeroLarge();
}

bool __fastcall BITSET::isZeroLarge()
{
    BITSETIMP * bitset = (BITSETIMP*)this;
    DWORD_PTR * thisDword = bitset->bitArray;
    do {
        if (*(thisDword++)) return false;
    } while (thisDword != bitset->arrayEnd);
    return true;
}



DWORD_PTR __fastcall BITSET::testBitLarge(unsigned bit)
{
    BITSETIMP * bitset = (BITSETIMP*)this;
    return (bitset->bitArray[bit >> BIT_SHIFT] & (((DWORD_PTR)1) << (bit & BIT_MASK)));    
}


DWORD_PTR __fastcall BITSET::testBits(unsigned start, unsigned size)
{
    if (size == 1) return testBit(start);

    if (((DWORD_PTR)this) & 1) {
        DWORD_PTR bits = (DWORD_PTR) this;
        return (bits | ~(GetMask(start, size))) == (DWORD_PTR) -1;
    }
    return testBitsLarge(start, size);

}

DWORD_PTR __fastcall BITSET::testBitsLarge(unsigned start, unsigned size)
{
    if (size == 1) return testBitLarge(start);

    BITSETIMP * bitset = (BITSETIMP*)this;
    
    unsigned index = start >> BIT_SHIFT;
    start = start & BIT_MASK;
    unsigned thisSize;

    while ((thisSize = min(size, MAX_SIZE - start))) {
        if ((bitset->bitArray[index] | ~(GetMask(start, thisSize))) != (DWORD_PTR) -1) {
            return 0;
        }

        if (size > thisSize)
            size -= thisSize;
        else 
            size = 0;
        start = 0;
        index ++;
    }

    return (DWORD_PTR) -1;

}

static const unsigned bitsPerNibble[] = 
{
    0,          // 0000
    1,          // 0001
    1,          // 0010
    2,          // 0011
    1,          // 0100
    2,          // 0101
    2,          // 0110
    3,          // 0111
    1,          // 1000
    2,          // 1001
    2,          // 1010
    3,          // 1011
    2,          // 1100
    3,          // 1101
    3,          // 1110
    4,          // 1111
};

unsigned __fastcall BITSET::numberSet(DWORD_PTR dw)
{
#define NUM_BITS(val) (bitsPerNibble[((val) & 0xF)])
#if defined(_M_IX86)
    // 8 Nibbles in a DWORD_PTR
    return NUM_BITS(dw      ) + NUM_BITS(dw >> 4 ) + NUM_BITS(dw >> 8 ) + NUM_BITS(dw >> 12) 
         + NUM_BITS(dw >> 16) + NUM_BITS(dw >> 20) + NUM_BITS(dw >> 24) + NUM_BITS(dw >> 28);

#else
    // NOTE: This assumes 8 bits per byte
    unsigned count = 0;
    for (int shift = 0; shift < (sizeof(DWORD_PTR) / 8); shift += 4)
        count += NUM_BITS(dw >> shift);
    return count;

#endif
#undef NUM_BITS
}

unsigned __fastcall BITSET::numberSet()
{
    if (((DWORD_PTR)this) & 1) {
        return numberSet((DWORD_PTR)this) - 1;
    }

    unsigned count = 0;
    for (DWORD_PTR *p = ((BITSETIMP*)this)->bitArray; p < ((BITSETIMP*)this)->arrayEnd; p += 1) {
        count += numberSet(*p);
    }
    return count;
}

BITSET * __fastcall BITSET::orInto(BITSET * source)
{
    if (((DWORD_PTR)this) & 1) {
        ASSERT(((DWORD_PTR)source) & 1);
        DWORD_PTR bits = (DWORD_PTR) this;
        bits |= ((DWORD_PTR) source);
        return (BITSET*) bits;
    }
    return orIntoLarge(source);
}

BITSET * __fastcall BITSET::orIntoLarge(BITSET * source)
{
    BITSETIMP * bitset = (BITSETIMP*)this;
    DWORD_PTR * thisDword = bitset->bitArray;
    DWORD_PTR * sourceDword = ((BITSETIMP*)source)->bitArray;
    do {
        *(thisDword++) |= *(sourceDword++);
    } while (thisDword != bitset->arrayEnd);
    return this;
}

BITSET * __fastcall BITSET::andInto(BITSET * source)
{
    if (((DWORD_PTR)this) & 1) {
        ASSERT(((DWORD_PTR)source) & 1);
        DWORD_PTR bits = (DWORD_PTR) this;
        bits &= ((DWORD_PTR) source);
        return (BITSET*) bits;
    }
    return andIntoLarge(source);
}

BITSET * __fastcall BITSET::andIntoLarge(BITSET * source)
{
    BITSETIMP * bitset = (BITSETIMP*)this;
    DWORD_PTR * thisDword = bitset->bitArray;
    DWORD_PTR * sourceDword = ((BITSETIMP*)source)->bitArray;
    do {
        *(thisDword++) &= *(sourceDword++);
    } while (thisDword != bitset->arrayEnd);
    return this;
}

BITSET * __fastcall BITSET::andInto(BITSET * source, DWORD_PTR * changed)
{
    if (((DWORD_PTR)this) & 1) {
        ASSERT(((DWORD_PTR)source) & 1);
        DWORD_PTR bits = (DWORD_PTR) this;
        bits &= ((DWORD_PTR) source); // compute the new bits
        *changed = ((DWORD_PTR)this & ~bits); // extinguished bits are those in old which are not in new
        return (BITSET*) bits;
    }
    return andIntoLarge(source, changed);
}

BITSET * __fastcall BITSET::andIntoLarge(BITSET * source, DWORD_PTR * changed)
{
    BITSETIMP * bitset = (BITSETIMP*)this;
    DWORD_PTR * thisDword = bitset->bitArray;
    DWORD_PTR * sourceDword = ((BITSETIMP*)source)->bitArray;
    *changed = 0;
    do {
        DWORD_PTR saved = *thisDword;   // save the old bits
        *thisDword &= *(sourceDword++); // compute the new bits
        *changed |= (saved & ~*(thisDword++));    // extinguished bits are those in old which are not in new
    } while (thisDword != bitset->arrayEnd);
    return this;
}

BITSET * __fastcall BITSET::create(unsigned size, NRHEAP * alloc, int init)
{
    if (size <= BIT_MASK) {
        return (BITSET*) ((DWORD_PTR)init | (DWORD_PTR)1);
    } else {
        return createLarge(size, alloc, init);
    }
}

BITSET * __fastcall BITSET::createCopy(BITSET * source, NRHEAP * alloc)
{
    if (((DWORD_PTR)source) & 1) {
        return source;
    } else {
        return createCopyLarge(source, alloc);
    }
}

BITSET * __fastcall BITSET::createCopy(BITSET * source, void * space)
{
    ASSERT(!(((DWORD_PTR)source) & 1));
    return createCopyLarge(source, space);
}


BITSET * __fastcall BITSET::createLarge(unsigned size, NRHEAP * alloc, int init)
{
    BITSETIMP * rval = (BITSETIMP*) alloc->Alloc(sizeof(BITSETIMP) + sizeof(DWORD_PTR) * ((size + MAX_SIZE) >> BIT_SHIFT));
    ASSERT(!(((DWORD_PTR)rval) & 1));
    memset(rval->bitArray, init,  ((size + MAX_SIZE) >> BIT_SHIFT) * sizeof(DWORD_PTR));
    rval->arrayEnd = rval->bitArray + ((size + MAX_SIZE) >> BIT_SHIFT);
    return (BITSET*) rval;    
}

unsigned __fastcall BITSET::getSize()
{
    ASSERT(!(((DWORD_PTR)this) & 1));
    BITSETIMP * bitset = (BITSETIMP*)this;

    return (unsigned) ((bitset->arrayEnd - bitset->bitArray) * sizeof(DWORD_PTR) + sizeof(BITSETIMP));
}

unsigned __fastcall BITSET::sizeInBytes()
{
    if (((DWORD_PTR)this) & 1) {
        return sizeof(DWORD_PTR);
    } else {
        BITSETIMP * bitset = (BITSETIMP*)this;
        return (unsigned) ((bitset->arrayEnd - bitset->bitArray) * sizeof(DWORD_PTR));
    }
}

unsigned __fastcall BITSET::rawSizeInBytes(unsigned numberOfBits)
{
    return ((numberOfBits + MAX_SIZE) >> BIT_SHIFT) * sizeof(DWORD_PTR);
}

BYTE *   __fastcall BITSET::rawBytes(BITSET **bitset)
{
    if (((DWORD_PTR)*bitset) & 1) {
        return (BYTE*)bitset;
    } else {
        return (BYTE*)((BITSETIMP*)(*bitset))->bitArray;
    }
}

BITSET * __fastcall BITSET::createCopyLarge(BITSET *source, NRHEAP * alloc)
{
    BITSETIMP * bitset = (BITSETIMP*)source;
    size_t size = bitset->arrayEnd - bitset->bitArray; // this is size in DWORD_PTRS
    BITSETIMP * rval = (BITSETIMP*) alloc->Alloc(sizeof(BITSETIMP) + sizeof(DWORD_PTR) * size);
    memcpy(rval->bitArray, bitset->bitArray , size * sizeof(DWORD_PTR));
    rval->arrayEnd = rval->bitArray + size;
    return (BITSET*) rval;

}

BITSET * __fastcall BITSET::createCopyLarge(BITSET *source, void * space)
{
    BITSETIMP * bitset = (BITSETIMP*)source;
    size_t size = bitset->arrayEnd - bitset->bitArray; // this is size in DWORD_PTRS
    BITSETIMP * rval = (BITSETIMP*) space;
    memcpy(rval->bitArray, bitset->bitArray , size * sizeof(DWORD_PTR));
    rval->arrayEnd = rval->bitArray + size;
    return (BITSET*) rval;
}


BITSET * __fastcall BITSET::setInto(BITSET * source)
{
    if (((DWORD_PTR)source) & 1) {
        return source;
    }

    return setIntoLarge(source);
}

BITSET * __fastcall BITSET::setInto(BITSET * source, NRHEAP * alloc)
{
    if (((DWORD_PTR)source) & 1) {
        return source;
    }
    return setIntoLarge(source, alloc);
}

BITSET * __fastcall BITSET::setInto(BITSET * source, NRHEAP * alloc, DWORD_PTR * changed)
{
    if (((DWORD_PTR)source) & 1) {
        *changed = ((DWORD_PTR)this ^ (DWORD_PTR)source);
        return source;
    }

    return setIntoLarge(source, alloc, changed);
}

BITSET * __fastcall BITSET::setInto(int value)
{
    if (((DWORD_PTR)this) & 1) {
        return (BITSET*) ((DWORD_PTR)value | 1);
    }

    return setIntoLarge(value);

}

BITSET * __fastcall BITSET::setIntoLarge(int value)
{
    BITSETIMP * bitset = (BITSETIMP*)this;
    ASSERT(!(((DWORD_PTR)bitset) & 1));
    size_t size = bitset->arrayEnd - bitset->bitArray; // this is size in DWORD_PTRS
    memset(bitset->bitArray, value,  size * sizeof(DWORD_PTR));
    return (BITSET*) bitset;    
}


BITSET * __fastcall BITSET::setIntoLarge(BITSET * source)
{
    ASSERT(!(((DWORD_PTR)this) & 1));
    ASSERT(this);

    BITSETIMP * bitset = (BITSETIMP*)source;
    size_t size = bitset->arrayEnd - bitset->bitArray; // this is size in DWORD_PTRS
    BITSETIMP * rval = (BITSETIMP*) this;

    memcpy(rval->bitArray, bitset->bitArray , size * sizeof(DWORD_PTR));
    rval->arrayEnd = rval->bitArray + size;
    return (BITSET*) rval;
}

BITSET * __fastcall BITSET::setIntoLarge(BITSET * source, NRHEAP * alloc, DWORD_PTR * changed)
{
    ASSERT(!(((DWORD_PTR)this) & 1));
    if (!this) {
        *changed = 0;
        return createCopyLarge(source, alloc);
    }
    
    BITSETIMP * bitset = (BITSETIMP*)source;
    size_t size = bitset->arrayEnd - bitset->bitArray; // this is size in DWORD_PTRS
    BITSETIMP * rval = (BITSETIMP*) this;
    *changed = memcmp(bitset->bitArray, rval->bitArray, size * sizeof(DWORD_PTR));
    memcpy(rval->bitArray, bitset->bitArray , size * sizeof(DWORD_PTR));
    rval->arrayEnd = rval->bitArray + size;
    return (BITSET*) rval;
}

BITSET * __fastcall BITSET::setIntoLarge(BITSET * source, NRHEAP * alloc)
{
    ASSERT(!(((DWORD_PTR)this) & 1));
    if (!this) {
        return createCopyLarge(source, alloc);
    }
    
    BITSETIMP * bitset = (BITSETIMP*)source;
    size_t size = bitset->arrayEnd - bitset->bitArray; // this is size in DWORD_PTRS
    BITSETIMP * rval = (BITSETIMP*) this;
    memcpy(rval->bitArray, bitset->bitArray , size * sizeof(DWORD_PTR));
    rval->arrayEnd = rval->bitArray + size;
    return (BITSET*) rval;
}
    

__forceinline void BITSET::swap(BITSET *** bs1, BITSET *** bs2)
{
    BITSET ** temp = *bs1;
    *bs1 = *bs2;
    *bs2 = temp;
}

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


void FUNCBREC::realizeStringConcat(EXPRCONCAT * expr)
{
    BOOL allStrings = true;  // Are all arguments to the concat strings?
    TYPESYM * typeString = getPDT(PT_STRING);
    METHSYM * method;
    SYM * verified;

    ASSERT(expr->flags & EXF_UNREALIZEDCONCAT);

    expr->flags &= ~EXF_UNREALIZEDCONCAT;

    unrealizedConcatCount --;

    // Scan the list of things being concatinated to see if they are 
    // all strings or not.
    EXPR * list = expr->list;
    for (;;) {
        if (list->kind == EK_LIST) {
            if (list->asBIN()->p1->type != typeString) {
                allStrings = false; 
                break;
            }
            list = list->asBIN()->p2;
        }
        else {
            if (list->type != typeString) {
                allStrings = false; 
            }
            break;
        }
    }

    // We compile this to 4 different possibilities, depending on the arguments:
    // 1. <= 4 string arguments.
    //     string.Concat(string, string) or string.Concat(string, string, string)
    // 2. > 4 string arguments
    //     string.Concat(string[])
    // 3. <= 3 arguments, at least one not a string
    //     string.Concat(object, object) or string.Concat(object, object, object)
    // 4. > 3 arguments, at least one not a stinrg
    //     string.Concat(object[])

    EXPR * args;
    if (expr->count > (allStrings ? 4U : 3U)) {
        // Create array from arguments.
        args = newExpr(NULL, EK_ARRINIT, compiler()->symmgr.GetArray(getPDT(allStrings ? PT_STRING : PT_OBJECT), 1));
        args->asARRINIT()->args = expr->list;
        args->asARRINIT()->dimSize = expr->count;
        args->asARRINIT()->dimSizes = &(args->asARRINIT()->dimSize);
        args->flags |= (expr->flags & EXF_VALONSTACK);
    } else {
        args = expr->list;
    }

    EXPR * obj = NULL;

    // Call static method String.Concat.
    method = compiler()->symmgr.LookupGlobalSym(
        compiler()->namemgr->GetPredefName(PN_STRCONCAT),
        typeString,
        MASK_METHSYM)->asMETHSYM();
    if (!method) {
        compiler()->clsDeclRec.errorStrStrFile(
                expr->tree, 
                inputfile, 
                ERR_MissingPredefinedMember, 
                getPDT(PT_STRING)->name->text, 
                compiler()->namemgr->GetPredefName(PN_STRCONCAT)->text);
    }

    verified = verifyMethodCall(NULL, method, args, &obj, 0);

    EXPRCALL * call = newExpr(NULL, EK_CALL, typeString)->asCALL();
    call->object = NULL;
    call->flags |= EXF_BOUNDCALL | (expr->flags & EXF_VALONSTACK);
    call->method = verified->asMETHSYM();
    call->args = args;

    expr->list = call;
}


bool FUNCBREC::realizeGoto(EXPRGOTO * expr)
{
    ASSERT(expr->flags & EXF_UNREALIZEDGOTO);
    expr->flags &= ~EXF_UNREALIZEDGOTO;
    if (expr->flags & EXF_BADGOTO) {
        return false;
    }
    unrealizedGotoCount --;

    SCOPESYM * scope = expr->currentScope;

    LABELSYM * target = NULL;
    bool hitFinally = false;
    if (expr->flags & EXF_GOTOCASE) {
        target = compiler()->symmgr.LookupLocalSym(expr->labelName, expr->targetScope, MASK_LABELSYM)->asLABELSYM();
    } else {
        do {
            target = compiler()->symmgr.LookupLocalSym(expr->labelName, scope, MASK_LABELSYM)->asLABELSYM();
            if (target) {
                expr->targetScope = scope;
                break;
            }
            if (scope->scopeFlags & SF_FINALLYSCOPE) {
                hitFinally = true;
            } else if (scope->scopeFlags & (SF_TRYSCOPE | SF_CATCHSCOPE)) {
                expr->flags |= EXF_GOTOASLEAVE;
            }
            scope = scope->parent->asSCOPESYM();
        } while (scope);
    }

    if (!target) {
        errorName(expr->tree, ERR_LabelNotFound, expr->labelName);
        return false;
    } else {
        if (!(expr->flags & EXF_GOTOCASE)) {
            target->targeted = true;
            untargetedLabelCount --;
            if (hitFinally) {
                errorN(expr->tree, ERR_BadFinallyLeave);
            }
            if (!target->labelExpr->checkedDupe) {
                target->labelExpr->checkedDupe = true;
                while ((scope = scope->parent->asSCOPESYM())) {
                    SYM * sym = compiler()->symmgr.LookupLocalSym(expr->labelName, scope, MASK_LABELSYM);
                    if (sym) {
                        errorName(expr->tree, ERR_LabelShadow, expr->labelName);
                    }
                }
            }
        }
        expr->label = target->labelExpr;
    }
    
    return true;

}


void FUNCBREC::reportUnreferencedVars(PSCOPESYM scope)
{
    SYM * current = scope->firstChild;
    while (current) {
        switch (current->kind) {
        case SK_LOCVARSYM:
            if (!current->asLOCVARSYM()->slot.isReferenced && !current->asLOCVARSYM()->slot.isParam) {
                warningSym(current->asLOCVARSYM()->declTree, current->asLOCVARSYM()->slot.isReferencedAssg ? WRN_UnreferencedVarAssg : WRN_UnreferencedVar, current);
            }
            break;    
        case SK_SCOPESYM:
            reportUnreferencedVars(current->asSCOPESYM());
            break;
        default:
            break;
        }
        current = current->nextChild;
    }
}

void FUNCBREC::postBindCompile(EXPRBLOCK * expr)
{
    if (unreferencedVarCount) reportUnreferencedVars(pOuterScope);

    if (
        !unreachedLabelCount && 
        !unrealizedConcatCount && 
        !unrealizedGotoCount && 
        !uninitedVarCount && 
        !untargetedLabelCount &&
        !reachablePass 
        ) {

        return;
    }
    
    TimerStart(TIME_BINDPARTTWO);

    bool anyUnrechableCode = reachablePass || unreachedLabelCount;
    
    currentBitset = BITSET::create(uninitedVarCount , allocator, 0);
    errorDisplayedBitset = BITSET::create(uninitedVarCount , allocator, 0);

    codeIsReachable = true;
    redoList = NULL;
    reachablePass = true;
    info->hasRetAsLeave = false;

    scanStatement(expr);

    if (codeIsReachable) {
        if (pMSym->retType == getVoidType()) {
            checkOutParams();
            if (expr) expr->flags |= EXF_NEEDSRET;
        } else {
           errorSym(pTree, ERR_ReturnExpected, pMSym);
        }
    }

    scanRedoList();

    reachablePass = false;


    // now, our variables were:

    // unreachedLabelCount
    // >0 means that some labels were define by the user but never reached...
    // only if all labels were reached can we know that all code was reached since we
    // assumed that all labels were reachable in the binding pass
    // so, we will error if this is >0

    // unrealizedConcatCount 
    // if this is >0 then we had some unreachable code, but since this is 
    // merely a sufficient indicator, not a necessary one, we can only
    // assert on it...

    // unrealizedGotoCount 
    // same as unrealizedConcatCount

    // uninitedVarCount 
    // we don't care what this is now, since it is never decremented...

    // untargetedLabelCount
    // if this is >0 then we had labels w/o reachable gotos pointing to them,
    // if unreachedLabelCount is 0 then we know that we saw all gotos, and
    // therefore this label was unreferenced...

    while (unreachedLabelCount > 0) {
        // some code was unreached... 
        
        EXPRLOOP(userLabelList, label)
            EXPRLABEL * l = label->asANYLABEL();
            if (!l->definitelyReachable) {
                l->definitelyReachable = true;
                unreachedLabelCount--;
                l->entranceBitset = BITSET::create(uninitedVarCount, allocator, -1);
                warningN(l->tree, WRN_UnreachableCode);
                addToRedoList(l->owningBlock, false);
                scanRedoList();
                break;
            }
        ENDLOOP;
    }

    EXPRGOTO * current = gotos;
    while (unrealizedGotoCount > 0) {
        while (!(current->flags & EXF_UNREALIZEDGOTO)) {
            current = current->prev;
        }
        realizeGoto(current);
    }

    // ok, now all unreachable spots have been errored out, and we may proceed
    // to unreferened labels:

    if (untargetedLabelCount > 0) {
        EXPRLOOP(userLabelList, label)
            EXPRLABEL * l = label->asANYLABEL();
            if (!l->checkedDupe) {
                warningN(l->tree, WRN_UnreferencedLabel);
            }
        ENDLOOP;
    }

    reachablePass = anyUnrechableCode;

    TimerStop(TIME_BINDPARTTWO);
}


void FUNCBREC::checkOutParams()
{
    if (thisPointer && thisPointer->slot.ilSlot && !thisPointer->type->asAGGSYM()->isEmptyStruct) {
        if (!currentBitset->testBits(thisPointer->slot.ilSlot, thisPointer->type->asAGGSYM()->getFieldsSize())) 
        {
            // Determine which fields of this were unassigned, and report error for each.
            for (SYM * field = thisPointer->type->asAGGSYM()->firstChild; field; field = field->nextChild) {
                if (field->kind == SK_MEMBVARSYM && !field->asMEMBVARSYM()->isStatic) {
                    if (!currentBitset->testBits(thisPointer->slot.ilSlot + field->asMEMBVARSYM()->offset, 
                                                 field->asMEMBVARSYM()->type->getFieldsSize()))
                        errorSym(pTree, ERR_UnassignedThis, field);
                }
            }
        }
    }

    int outParams = outParamCount;
    SYM * current = pOuterScope->firstChild;

    while (outParams) {
        switch (current->kind) {
        case SK_LOCVARSYM:
            if (current->asLOCVARSYM()->slot.ilSlot) {
                outParams--;
                if (!currentBitset->testBits(current->asLOCVARSYM()->slot.ilSlot, current->asLOCVARSYM()->type->getFieldsSize())) {
                    errorSym(pTree, ERR_ParamUnassigned, current);
                }
            }
        default:
            break;
        }
        current = current->nextChild;
    }
}

unsigned __fastcall FUNCBREC::localIsUsed(LOCVARSYM * local)
{
    if (reachablePass) {
        return local->slot.isPBUsed;
    } else {
        return local->slot.isUsed;
    }
}


// Force is a boolean which indicates whether to add a block even though it
// is currently being scanned.  It should be set to true only on a 
// backward jump into that block.
void __fastcall FUNCBREC::addToRedoList(EXPRBLOCK *expr, int force) {
    if (!expr) return;

    if (expr->redoBlock) return;

    if (expr->scanning && !force) return;

    expr->redoBlock = true;

    if (!redoList) {
        redoList = expr;
    } else {
        redoList = newExprBinop(NULL, EK_LIST, NULL, expr, redoList);
    }
}

#if defined(_MSC_VER)
#pragma optimize ("w", off)
#endif  // defined(_MSC_VER)
void FUNCBREC::scanRedoList(EXPRBLOCK * excludedBlock)
{
    while (redoList) {

        EXPR * item = redoList;
        if (item->kind == EK_LIST) {
            redoList = item->asBIN()->p2;
            item = item->asBIN()->p1->asBLOCK();
        } else {
            redoList = NULL;
        }

        if (item == excludedBlock) continue;

        codeIsReachable = true;
        if (!item->asBLOCK()->enterBitset) {
            item->asBLOCK()->enterBitset = BITSET::create(uninitedVarCount, allocator, -1);
        }
        currentBitset = currentBitset->setInto(item->asBLOCK()->enterBitset);
        scanStatement(item);
    }
}
#if defined(_MSC_VER)
#pragma optimize ("", on)
#endif  // defined(_MSC_VER)

void __fastcall FUNCBREC::markAsUsed(EXPR * expr)
{

    LOCVARSYM * local;

AGAIN:
    if (!expr) return;

    switch (expr->kind) {
    case EK_LOCAL:

        local = expr->asLOCAL()->local;
        ASSERT(!local->isConst);
        if (!local->slot.isParam) {
            if (!local->slot.isPBUsed) {
                local->slot.isPBUsed = reachablePass;
            }
        }
        break;
    case EK_FIELD:
        expr = expr->asFIELD()->object;
        goto AGAIN;
    default:
        break;
    }
}


void __fastcall FUNCBREC::checkOwnerOffset(EXPRFIELD * expr)
{
    unsigned offset;
    if ((offset = expr->ownerOffset) && (expr->object->kind != EK_LOCAL || expr->object->asLOCAL()->local != thisPointer || !pMSym || !pMSym->isCtor)) {
        if (!currentBitset->testBit(offset)) {
            currentBitset = currentBitset->setBit(offset);
            
            EXPR * object = expr->object;
            while (object->kind != EK_LOCAL) {
                object = object->asFIELD()->object;
            }
            object->asLOCAL()->local->slot.needsPreInit = true;
            info->preInitsNeeded = true;

        }
    }
}

void __fastcall FUNCBREC::scanExpression(EXPR * expr, BITSET ** trueSet, BITSET ** falseSet, bool sense)
{
    
    EXPR * op1, *op2, *op3;
    unsigned slot;
    BITSET * TSop1, *FSop1, *TSop2, *FSop2;
    LOCVARSYM * local;
    unsigned short offset, size;

START:

    if (!expr) return;

    if (!sense) {
        if (trueSet) {
            BITSET::swap(&trueSet, &falseSet);
        }
        sense = true; 
    }

    switch(expr->kind) {
    case EK_CONSTANT:
        if (trueSet) {
            if (!expr->asCONSTANT()->getVal().iVal) {
                BITSET::swap(&trueSet, &falseSet);
            }
            *trueSet = (*trueSet)->setInto(currentBitset);
            *falseSet = (*falseSet)->setInto(-1);
            return;
        }
        return;
    case EK_WRAP:
    case EK_FUNCPTR:
    case EK_TYPEOF:
    case EK_SIZEOF:
        break;
    case EK_LOCAL:
        markAsUsed(expr);
        local = expr->asLOCAL()->local;
        // We ignore outparams since they get assigned at the conclusion of the call expr
        if ((slot = local->slot.ilSlot) && (expr->type->kind != SK_PARAMMODSYM || expr->type->asPARAMMODSYM()->isRef)) {
            if (!currentBitset->testBits(slot, size = local->type->getFieldsSize())) {
                if (!errorDisplayedBitset->testBits(slot, size)) {
                    errorDisplayedBitset = errorDisplayedBitset->setBits(slot, size);
                    if (local == thisPointer) {
                        errorN(expr->tree, ERR_UseDefViolationThis);
                    } else {
                        errorSym(expr->tree, ERR_UseDefViolation, expr->asLOCAL()->local);
                    }
                }
            }
        }
        break;
    case EK_ARRINIT:
        expr = expr->asARRINIT()->args;
        goto AGAIN;
    case EK_CAST:
        expr = expr->asCAST()->p1;
        goto AGAIN;
    case EK_CONCAT:
        if (expr->flags & EXF_UNREALIZEDCONCAT) {
            realizeStringConcat(expr->asCONCAT());
        }
        expr = expr->asCONCAT()->list;
        goto AGAIN;
    case EK_ZEROINIT:
        if (!(op1 = expr->asZEROINIT()->p1) || !assignToExpr(op1, NULL)) {
            scanExpression(expr->asZEROINIT()->p1);
        }
        break;
    case EK_USERLOGOP:
        scanExpression(expr->asUSERLOGOP()->opX);
        scanExpression(expr->asUSERLOGOP()->callOp->asCALL()->args->asBIN()->p2);
        break;
    case EK_CALL:
        if (expr->flags & EXF_NEWSTRUCTASSG) {
            // This assigns to the object being called on...
            if (assignToExpr(expr->asCALL()->object, expr->asCALL()->args)) {
                goto DOOUTPARAMS;
            }
        }
        scanExpression(expr->asCALL()->object);
        scanExpression(expr->asCALL()->args);
DOOUTPARAMS:
        if (expr->flags & EXF_HASREFPARAM) {
            // Now, we want to look over the args again and assign to any out params...
            EXPRLOOP(expr->asCALL()->args, arg) 
                if (arg->type->kind == SK_PARAMMODSYM) {
                    if (arg->type->asPARAMMODSYM()->isOut) {
                        if (arg->kind == EK_LOCAL) {
                            if ((slot = arg->asLOCAL()->local->slot.ilSlot) != 0) {
                                if (!currentBitset->testBit(slot)) {
                                    arg->asLOCAL()->local->slot.needsPreInit = true;
                                    info->preInitsNeeded = true;
                                }
                            }
                        }
                    }
                    assignToExpr(arg, NULL);
                }
            ENDLOOP;
        }
        break;
    case EK_NOOP:
        break;
    case EK_MULTI:
        scanExpression(expr->asMULTI()->left);
        expr = expr->asMULTI()->op;
        goto AGAIN;
    case EK_PROP:
        scanExpression(expr->asPROP()->object);
        expr = expr->asPROP()->args;
        goto AGAIN;
    case EK_FIELD: 
        checkOwnerOffset(expr->asFIELD());
        if ((offset = expr->asFIELD()->offset)) {
            if (!(expr->flags & EXF_MEMBERSET)) {
                if (!currentBitset->testBits(offset, size = expr->asFIELD()->field->type->getFieldsSize())) {
                    if (!errorDisplayedBitset->testBits(offset, size)) {
                        errorDisplayedBitset = errorDisplayedBitset->setBits(offset, size);
                        errorName(expr->tree, ERR_UseDefViolationField, expr->asFIELD()->field->name);
                    }
                }
            }
            markAsUsed(expr->asFIELD()->object);
            break;
        }
        expr = expr->asFIELD()->object;
        goto AGAIN;
    case EK_LOGOR:
        ASSERT(sense);
        sense = false;
        if (trueSet) {
            BITSET::swap(&trueSet, &falseSet);
        }

    case EK_LOGAND: 

        op1 = expr->asBIN()->p1;
        op2 = expr->asBIN()->p2;

        if (trueSet) {

            TSop1 = *trueSet;
            FSop1 = *falseSet;
        } else {
            createBitsetOnStack(TSop1, currentBitset);
            createBitsetOnStack(FSop1, currentBitset);
        }

        scanExpression(op1, &TSop1, &FSop1, sense);

        createBitsetOnStack(FSop2, TSop1);
        currentBitset = currentBitset->setInto(TSop1);
        scanExpression(op2, &TSop1, &FSop2, sense);
        FSop1 = FSop1->andInto(FSop2);

        goto JOINSETS;

    
    case EK_QMARK:
        op1 = expr->asBIN()->p1;
        op2 = expr->asBIN()->p2;
        op3 = op2->asBIN()->p2;
        op2 = op2->asBIN()->p1;

        // so we have op1 ? op2 : op3 

        if (trueSet) {
            TSop1 = *trueSet;
            FSop1 = *falseSet;
        } else {
            createBitsetOnStack(TSop1, currentBitset);
            createBitsetOnStack(FSop1, currentBitset);
        }

        scanExpression(op1, &TSop1, &FSop1);

        currentBitset = currentBitset->setInto(TSop1);
        createBitsetOnStack(FSop2, currentBitset);
        
        scanExpression(op2, &TSop1, &FSop2);

        currentBitset = currentBitset->setInto(FSop1);
        createBitsetOnStack(TSop2, currentBitset);

        scanExpression(op3, &TSop2, &FSop1);

        TSop1 = TSop1->andInto(TSop2);
        FSop1 = FSop1->andInto(FSop2);

JOINSETS:

        if (!trueSet) {
            currentBitset = currentBitset->setInto(TSop1);
            currentBitset = currentBitset->andInto(FSop1);
        } else {
            *trueSet = TSop1;
            *falseSet = FSop1;
        }

        return;
    case EK_LOGNOT:
        op1 = expr->asBIN()->p1;
        sense = !sense;
        expr = op1;
        goto START;

    case EK_ADDR:
        ASSERT(expr->asBIN()->p2 == NULL);
    case EK_ASSG:
        op1 = expr->asBIN()->p1;
        op2 = expr->asBIN()->p2;
        if (assignToExpr(op1, op2)) {
            break;
        }
    default:
        ASSERT(expr->flags & EXF_BINOP);
        scanExpression(expr->asBIN()->p1);
        expr = expr->asBIN()->p2;
        goto AGAIN;
    }

DONE:
    if (trueSet) {
        *trueSet = (*trueSet)->setInto(currentBitset);
        *falseSet = (*falseSet)->setInto(currentBitset);
    }

    return;

AGAIN:

    if (trueSet) {
        scanExpression(expr);
        goto DONE;
    } else {
        goto START;
    }
}

bool __fastcall FUNCBREC::assignToExpr(EXPR * expr, EXPR * val)
{
    unsigned slot;
    LOCVARSYM * local;
    TYPESYM * type;

    if (expr->kind == EK_LOCAL) {
        scanExpression(val);
        local = expr->asLOCAL()->local;
        ASSERT(!local->isConst);
        if ((slot = local->slot.ilSlot)) {
            if ((type = local->type)->fundType() == FT_STRUCT) {
                currentBitset = currentBitset->setBits(slot, type->asAGGSYM()->getFieldsSize());
            } else {
                currentBitset = currentBitset->setBit(slot);
            }
        }
        if ((slot = local->ownerOffset)) {
            currentBitset = currentBitset->setBit(slot);
        }
        return true;
    } else if (expr->kind == EK_FIELD && (slot = expr->asFIELD()->offset)) {
        bool fieldInConstructor = expr->asFIELD()->object->kind == EK_LOCAL && expr->asFIELD()->object->asLOCAL()->local == thisPointer && thisPointer && pMSym && pMSym->isCtor;

        if (!fieldInConstructor) {
            checkOwnerOffset(expr->asFIELD());
        }
        scanExpression(val);
        if (fieldInConstructor) {
            currentBitset = currentBitset->setBit(expr->asFIELD()->ownerOffset);
        }
        if ((type = expr->asFIELD()->field->type)->fundType() == FT_STRUCT) {
            currentBitset = currentBitset->setBits(slot, type->asAGGSYM()->getFieldsSize());
        } else {
            currentBitset = currentBitset->setBit(slot);
        }
        return true;
    }
    return false;
}

// return true if something changed (like end-reachability or set of defined locals...)
void __fastcall FUNCBREC::visitFinallyBlock(FINALLYSTORE * store, EXPRBLOCK * block)
{

    // if we are entering w/ the same entry set as before, then skip it since we already know the
    // answer...
    if (currentBitset->isEqual(store->entry)) {
        currentBitset->setInto(store->exit);
        codeIsReachable = store->reachableEnd;
        return;
    }

    // First, save off what the block's entry & exit points are:

    FINALLYSTORE saved;
    bool didSave = false;
    if (block->enterBitset) {
        createBitsetOnStack(saved.entry, block->enterBitset);
        createBitsetOnStack(saved.exit, block->exitBitset);
        didSave = true;
    }
    saved.reachableEnd = block->exitReachable;

    // save the redo list...
    EXPR * savedRedoList = redoList;
    redoList = NULL;

    EXPRLOOP(savedRedoList, redoBlock)
        redoBlock->asBLOCK()->redoBlock = false;
    ENDLOOP;

    // force the block to be traversed (the block will be initied w/ the current set by the scanner)
    block->redoBlock = true;

    //run the finally
    scanStatement(block);
    // run redo blocks, but don't go above the finally block
    scanRedoList(block->owningBlock);

    // restore the redo list
    redoList = savedRedoList;
    EXPRLOOP(savedRedoList, redoBlock)
        redoBlock->asBLOCK()->redoBlock = true;
    ENDLOOP;

    // set the newly found out exit set
    if (block->enterBitset) {
        store->entry = store->entry->setInto(block->enterBitset, allocator);
        store->exit = store->exit->setInto(block->exitBitset, allocator);
        currentBitset = currentBitset->setInto(block->exitBitset);
        codeIsReachable = store->reachableEnd = block->exitReachable;
    }

    if (didSave) {
        // reset the block's saved state
        block->exitBitset = block->exitBitset->setInto(saved.exit);
        block->enterBitset = block->enterBitset->setInto(saved.entry);
        block->exitReachable = saved.reachableEnd;
    } else {
        block->exitBitset = block->enterBitset = NULL;
        block->exitReachable = false;
    }

}

// return true if all you can pass through the leaves, false if code following is unreachable
bool __fastcall FUNCBREC::onLeaveReached(EXPR * expr)
{
    ASSERT(expr->kind == EK_GOTO || expr->kind == EK_RETURN);
    SCOPESYM * current;
    SCOPESYM * target;
    FINALLYSTORE * storeList;
    if (expr->kind == EK_GOTO) {
        current = expr->asGOTO()->currentScope;
        target = expr->asGOTO()->targetScope;
        storeList = expr->asGOTO()->finallyStore;
    } else {
        current = expr->asRETURN()->currentScope;
        target = NULL;
        storeList = expr->asRETURN()->finallyStore;
    }
    ASSERT(current);


    while (current != target) {
        if (current->scopeFlags & SF_TRYOFFINALLY) {
            EXPRBLOCK * block = current->finallyScope->block;
            ASSERT(block);
            visitFinallyBlock(storeList++, block);
            if (!codeIsReachable) return false;
        }
        current = current->parent->asSCOPESYM();
    }

    return true;

}


void __fastcall FUNCBREC::onLabelReached(EXPR *expr, BITSET * set)
{

    EXPRLABEL *label;
    DWORD_PTR changed;
    int force;

    if (expr->kind != EK_LABEL && expr->kind != EK_SWITCHLABEL) {
        label = expr->asGT()->label;
        // we don't care about switch labels, they are always reachable...
        // and we never learn anything about them either...
        if (label->kind == EK_SWITCHLABEL && !label->userDefined ) return;

        // Is this the the first time we are seeing this label?
        if (!label->entranceBitset) {
            // if so, then just set its bitset, and mark it as changed...
            label->entranceBitset = BITSET::createCopy(set, allocator);
            changed = 1;
        } else {
            label->entranceBitset = label->entranceBitset->andInto(set, &changed);
        }

        // a backward goto always requires a reexamination of the target block
        // A forward goto, however, does so only if the block isn't being examined
        // already, in which case this label will be hit later anyway...
        force = !(expr->flags & EXF_GOTOFORWARD);

    } else {
        label = expr->asANYLABEL();
        // ok, in this case we reached this label by fallthrough,
        // so, just update the label's set:

        if (label->entranceBitset) {
            label->entranceBitset = label->entranceBitset->andInto(set);
        } else {
            label->entranceBitset = BITSET::createCopy(set, allocator);
        }

        changed = 0;
        force = 0;
    }

    // if we have not seen this label before
    if (!label->definitelyReachable) {
        // then we have now, and count it as reached...
        label->definitelyReachable = true;
        if (label->userDefined) {
            unreachedLabelCount--;
        }
    
        changed |= (label != expr);
        // meaning, that we reached this through a goto,
        // we should rescan the owning block since the label's reachability changed

    }

    if (changed) {
        addToRedoList(label->owningBlock, force);
    }
}

void __fastcall FUNCBREC::scanStatement(EXPR * expr)
{

    BITSET * TS, *FS;
    DWORD_PTR changed;
    bool isConstant;
    EXPRBLOCK * block;

AGAIN:
    if (!expr) return;
    
    switch (expr->kind) {
    case EK_HANDLER:
        codeIsReachable = true; 
        // an exception or finally handler is always reachable...
        expr = expr->asHANDLER()->handlerBlock;
        goto AGAIN;
    case EK_SWITCHLABEL:
        ASSERT(expr->asSWITCHLABEL()->userDefined);
    case EK_LABEL:
        if (codeIsReachable) {
            onLabelReached(expr, currentBitset);
        } else if (expr->asANYLABEL()->definitelyReachable) {
            codeIsReachable = true;
        }
        if (codeIsReachable) {
            currentBitset = currentBitset->setInto(expr->asANYLABEL()->entranceBitset);
        }
        if (expr->kind == EK_SWITCHLABEL) {

            EXPRSWITCHLABEL * label = expr->asSWITCHLABEL();

            scanStatement(label->statements);

            if (codeIsReachable && !label->fellThrough && reachablePass) {
                errorName(label->tree, ERR_SwitchFallThrough, getSwitchLabelName(label->key->asCONSTANT()));
                label->fellThrough = true;
            }
        }
            
        return;
    case EK_LIST:
        scanStatement(expr->asBIN()->p1);
        expr = expr->asBIN()->p2;
        goto AGAIN;
    default:
        break;
    }
    
    if (!codeIsReachable && expr->kind != EK_SWITCH) {
        return;
    }
    
    switch (expr->kind) {
    case EK_DECL:
        scanExpression(expr->asDECL()->init);
        return;
    case EK_BLOCK:
        block = expr->asBLOCK();
        block->enterBitset = block->enterBitset->setInto(currentBitset, allocator, &changed);
        if (block->redoBlock || changed) {
            block->scanning = true;
            block->redoBlock = false;
            scanStatement(block->statements);
            block->scanning = false;
            block->exitBitset = block->exitBitset->setInto(currentBitset, allocator, &changed);
            if ((codeIsReachable != block->exitReachable) || changed) {
                addToRedoList(block->owningBlock, false);
            }
            block->exitReachable = codeIsReachable;
        } else {
            currentBitset = currentBitset->setInto(block->exitBitset);
            codeIsReachable = expr->asBLOCK()->exitReachable;
        }
        return;
    case EK_STMTAS:
        scanExpression(expr->asSTMTAS()->expression);
        break;
    case EK_THROW:
        scanExpression(expr->asTHROW()->object);
        codeIsReachable = false;
        currentBitset = currentBitset->setInto(-1);
        break;
    case EK_RETURN:
        scanExpression(expr->asRETURN()->object);
        if (expr->flags & EXF_RETURNASLEAVE) {
            info->hasRetAsLeave |= reachablePass;
            if (expr->flags & EXF_RETURNASFINALLYLEAVE) {
                if (!onLeaveReached(expr)) {
                    expr->flags |= EXF_RETURNBLOCKED;
                    goto RETURN_UNREACHABLE;
                }
            }
        }
        checkOutParams();
RETURN_UNREACHABLE:
        codeIsReachable = false;
        currentBitset = currentBitset->setInto(-1);
        break;
    case EK_GOTOIF:
        createBitsetOnStack(TS, currentBitset);
        createBitsetOnStack(FS, currentBitset);
        scanExpression(expr->asGOTOIF()->condition, &TS, &FS, expr->asGOTOIF()->sense);
        if (!expr->asGOTOIF()->condition->isFalse(expr->asGOTOIF()->sense)) {
            onLabelReached(expr, TS);
            if (expr->asGOTOIF()->condition->isTrue(expr->asGOTOIF()->sense)) {
                // the falseSet is all 1s here...
                codeIsReachable = false;
            }
        }
        currentBitset = currentBitset->setInto(FS);
        break;
    case EK_ASSERT:
        ASSERT(!(expr->flags & EXF_GOTOASLEAVE));
        onLabelReached(expr, currentBitset);
        createBitsetOnStack(TS, currentBitset);
        createBitsetOnStack(FS, currentBitset);
        scanExpression(expr->asASSERT()->expression, &TS, &FS, expr->asASSERT()->sense);
        currentBitset = currentBitset->setInto(TS);
        codeIsReachable = true;
        expr->kind = EK_GOTO;
        break;
    case EK_GOTO:
        if (expr->flags & EXF_UNREALIZEDGOTO) {
            if (!realizeGoto(expr->asGOTO())) {
                expr->flags |= EXF_UNREALIZEDGOTO | EXF_BADGOTO;
                codeIsReachable = false;
                currentBitset = currentBitset->setInto(-1);
                break;
            }
        }
        if (expr->flags & EXF_GOTOASFINALLYLEAVE) {
            if (!onLeaveReached(expr)) {
                expr->flags |= EXF_GOTOBLOCKED;
                goto GOTO_UNREACHABLE;
            }
        }
        onLabelReached(expr, currentBitset);
GOTO_UNREACHABLE:
        codeIsReachable = false;
        currentBitset = currentBitset->setInto(-1);
        break;
    case EK_SWITCH: {
        if (expr->asSWITCH()->arg->kind == EK_CONSTANT) {

            // we treat this just like a series of labels w/ code

            expr = expr->asSWITCH()->bodies;
            goto AGAIN;

        } else {

            if (!codeIsReachable) return;

            scanExpression(expr->asSWITCH()->arg);

            isConstant = expr->asSWITCH()->arg->kind == EK_CONSTANT;

            BITSET * beforeCases = BITSET::createCopy(currentBitset, allocator);
            BITSET * afterCases;
            if (expr->flags & EXF_HASDEFAULT) {
                afterCases = BITSET::create(uninitedVarCount, allocator, -1);
            } else {
                afterCases = NULL;
            }

            EXPRLOOP(expr->asSWITCH()->bodies, switchLabel)
                EXPRSWITCHLABEL * label = switchLabel->asSWITCHLABEL();

                codeIsReachable = true;

                if (label->statements) {

                    currentBitset = currentBitset->setInto(beforeCases);

                    scanStatement(label->statements);

                    // if we have an unreported fall-through, error on it:
                    if (codeIsReachable && !label->fellThrough && reachablePass) {
                        errorName(label->tree, ERR_SwitchFallThrough, getSwitchLabelName(label->key->asCONSTANT()));
                        label->fellThrough = true;
                    }

                    if (afterCases) {
                        afterCases = afterCases->andInto(currentBitset);
                    }
                }
            
            ENDLOOP;

            if (afterCases) { // ie, we have a default label...
                currentBitset = currentBitset->setInto(afterCases);
            } else {
                // ie, we don't have a default, so a fall through w/o hitting any
                // cases is possible:
                currentBitset = currentBitset->setInto(beforeCases);
                codeIsReachable = true;
            }
        }

        return;
                    }
    case EK_TRY: {
        BITSET * beforeTry = BITSET::createCopy(currentBitset, allocator);
        scanStatement(expr->asTRY()->tryblock);
        BITSET * afterTry = BITSET::createCopy(currentBitset, allocator);
        bool tryReachable = codeIsReachable;
        if (expr->flags & EXF_ISFINALLY) {
            expr->flags &= ~EXF_FINALLYBLOCKED;  // in case it was set incorrectly in the bind phase...
            codeIsReachable = true; // by an exception, if nothing else...
            currentBitset = currentBitset->setInto(beforeTry);

            visitFinallyBlock(expr->asTRY()->finallyStore, expr->asTRY()->handlers->asBLOCK());

            // here, we have the union of after try and after handler...
            currentBitset = currentBitset->orInto(afterTry);

            if (!codeIsReachable) {
                expr->flags |= EXF_FINALLYBLOCKED;
            }

            codeIsReachable = codeIsReachable && tryReachable;
        } else {
            EXPRLOOP(expr->asTRY()->handlers, handler)
                codeIsReachable = true; // a catch is always reachable by an exception
                currentBitset = currentBitset->setInto(beforeTry);
                scanStatement(handler);
                afterTry = afterTry->andInto(currentBitset);
            tryReachable |= codeIsReachable;
            ENDLOOP;
            currentBitset = currentBitset->setInto(afterTry);
            codeIsReachable = tryReachable;
        }
        return;
                 }
    default:
        ASSERT(!"bad expr type");
    }

}
