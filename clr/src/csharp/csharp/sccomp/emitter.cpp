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
// File: emitter.cpp
//
// Routines for emitting CLR metadata and creating executable files.
// ===========================================================================

#include "stdafx.h"

#include "compiler.h"
#include <limits.h>
#include "fileiter.h"

#define SIGBUFFER_GROW 256      // Number of bytes to grow signature buffer by.

/*
 * Constructor
 */
EMITTER::EMITTER()
: tokrefHeap(compiler())
{
#if DEBUG
    sigBufferInUse = false;
#endif
    sigBuffer = NULL;

    tokrefList = NULL;
    iTokrefCur = CTOKREF;   // Signal that we must allocate a new block right away.

    cSecAttrSave = 0;
    listSecAttrSave = NULL;
}

/*
 * Destructor
 */
EMITTER::~EMITTER()
{
    Term();
}

/*
 * Terminate everything.
 */
void EMITTER::Term()
{
    FreeSavedSecurityAttributes();

    // Free the signature buffer.
    if (sigBuffer) {
#ifdef DEBUG
        sigBufferInUse = false;
#endif
        compiler()->globalHeap.Free(sigBuffer);
        sigBuffer = NULL;
    }

    tokrefHeap.Mark( &mark);
    EraseEmitTokens();  // Forget all tokens in this output file.
    tokrefHeap.FreeHeap();
}

/*
 * Semi-generic method to check for failure of a meta-data method.
 */
__forceinline void EMITTER::CheckHR(HRESULT hr)
{
    if (FAILED(hr))
        MetadataFailure(hr);
    SetErrorInfo(0, NULL); // Clear out any stale ErrorInfos
}

/*
 * Semi-generic method to check for failure of a meta-data method. Passed
 * an error ID in cases where the generic one is no good.
 */
__forceinline void EMITTER::CheckHR(int errid, HRESULT hr)
{
    if (FAILED(hr))
        MetadataFailure(errid, hr);
    SetErrorInfo(0, NULL); // Clear out any stale ErrorInfos
}

/*
 * Semi-generic method to check for failure of a debug method.
 */
__forceinline void EMITTER::CheckHRDbg(HRESULT hr)
{
    if (FAILED(hr))
        DebugFailure(hr);
    SetErrorInfo(0, NULL); // Clear out any stale ErrorInfos
}

/*
 * Handle a generic meta-data API failure.
 */
void EMITTER::MetadataFailure(HRESULT hr)
{
    MetadataFailure(FTL_MetadataEmitFailure, hr);
}

/*
 * Handle a generic debug API failure.
 */
void EMITTER::DebugFailure(HRESULT hr)
{
    WCHAR filename[MAX_PATH];

    CPEFile::GetPDBFileName(compiler()->curFile->GetOutFile(), filename);
    compiler()->Error(NULL, FTL_DebugEmitFailure, compiler()->ErrHR(hr), filename);
}

/*
 * Handle an API failure. The passed error code is expected to take one insertion string
 * that is filled in with the HRESULT.
 */
void EMITTER::MetadataFailure(int errid, HRESULT hr)
{
    compiler()->Error(NULL, errid, compiler()->ErrHR(hr), compiler()->curFile->GetOutFile()->name->text);
}


/*
 * Begin emitting an output file. If an error occurs, it is reported to the user
 * and then false is returned.
 */
bool EMITTER::BeginOutputFile()
{
    // cache a local copy, don't addRef because we don't own these!
    metaemit = compiler()->curFile->GetEmit();
    metaassememit = compiler()->curFile->GetAssemblyEmitter();
    debugemit = compiler()->curFile->GetDebugEmit();

#ifdef DEBUG
    // In debug, set a handler to call back on us just to make sure it is never called!
    metaemit->SetHandler(this);
#endif //DEBUG

    // Initially, no token is set.
    tokrefHeap.Mark( &mark);

    return true;
}


/*
 * End writing an output file. If true is passed, the output file is actually written.
 * If false is passed (e.g., because an error occurred), the output file is not
 * written.
 *
 * true is returned if the output file was successfully written.
 */
bool EMITTER::EndOutputFile(bool writeFile)
{
    OUTFILESYM *outfile = compiler()->curFile->GetOutFile();

    //
    // do module attributes
    //
    if (compiler()->IsIncrementalRebuild()) {
        //
        // delete existing module attributes
        //
        DeleteCustomAttributes(outfile->firstInfile(), GetModuleToken());
    }
    GLOBALATTRBIND::Compile(compiler(), outfile, outfile->attributes, GetModuleToken());

    if (compiler()->ErrorCount()) {
        writeFile = false;
    }

    bool fDll = outfile->isDll;

    PAL_TRY {

        if (fDll) {
            outfile->entrySym = NULL;  // Dlls don't have entry points.
        }
        else {
            if (outfile->entrySym == NULL && writeFile) {
                ASSERT (!outfile->isUnnamed());
                compiler()->Error(NULL, ERR_NoEntryPoint, outfile->name->text);
                writeFile = false;
            }
        }

        if (writeFile && !compiler()->options.m_fNOCODEGEN) {
            
            // Set output file attributes.
            compiler()->curFile->SetAttributes(fDll);

            // The rest is done by PEFile
        }
    }
    PAL_FINALLY {
        // This needs to always happen, even if writing failed.

        EraseEmitTokens();  // Forget all tokens in this output file.
        metaemit = NULL;
        debugemit = NULL;

        // Free any IDocumentWriter interfaces for this output file.
        for (PINFILESYM pInfile = compiler()->curFile->GetOutFile()->firstInfile(); pInfile != NULL; pInfile = pInfile->nextInfile()) {
            if (! pInfile->isSource)
                continue;                   // Not a source file.

            if (pInfile->documentWriter) {
                pInfile->documentWriter->Release();
                pInfile->documentWriter = NULL;
            }
        }
    }
    PAL_ENDTRY

    return writeFile ? true : false;  // Never return true if writeFile was false.
}


/*
 * Begin emitting a signature. Returns a pointer to the signature that
 * should be passed to other emitting routines.
 */
PCOR_SIGNATURE EMITTER::BeginSignature()
{
    ASSERT(!sigBufferInUse);
#ifdef DEBUG
    sigBufferInUse = true;
#endif

    // If needed, create the initial allocation of the signature buffer.
    if (!sigBuffer) {
        sigBuffer = (PCOR_SIGNATURE) compiler()->globalHeap.Alloc(SIGBUFFER_GROW);
        sigEnd = sigBuffer + SIGBUFFER_GROW;
    }

    return sigBuffer;
}

/*
 * Emit a byte to the current signature.
 */
__forceinline PCOR_SIGNATURE EMITTER::EmitSignatureByte(PCOR_SIGNATURE curSig, BYTE b)
{
    ASSERT(sigBufferInUse);

    if (curSig + sizeof(BYTE) > sigEnd)
        curSig = GrowSignature(curSig);
    *curSig++ = b;
    return curSig;
}

/*
 * Emit an encoded interger to the current signature.
 */
__forceinline PCOR_SIGNATURE EMITTER::EmitSignatureUInt(PCOR_SIGNATURE curSig, ULONG u)
{
    ASSERT(sigBufferInUse);

    if (curSig + sizeof(DWORD) > sigEnd)
        curSig = GrowSignature(curSig);
    curSig += CorSigCompressData(u, curSig);
    return curSig;
}

/*
 * Emit a token to the current signature.
 */
__forceinline PCOR_SIGNATURE EMITTER::EmitSignatureToken(PCOR_SIGNATURE curSig, mdToken token)
{
    ASSERT(sigBufferInUse);

    if (curSig + sizeof(DWORD) > sigEnd)
        curSig = GrowSignature(curSig);

    curSig += CorSigCompressToken(token, curSig);
    return curSig;
}

/*
 * Finish emitting a signature. Returns the signature start and size.
 */
__forceinline PCOR_SIGNATURE EMITTER::EndSignature(PCOR_SIGNATURE curSig, int * cbSig)
{
    ASSERT(sigBufferInUse);
#ifdef DEBUG
    sigBufferInUse = false;
#endif

    *cbSig = (int)(curSig - sigBuffer);
    return sigBuffer;
}

/*
 * Ensures the signature buffer has a minimum size
 */
PCOR_SIGNATURE EMITTER::EnsureSignatureSize(ULONG cb)
{
    ASSERT(sigBufferInUse);

    ULONG cbAlloced = (ULONG)(sigEnd - sigBuffer);        // ammount allocated.

    if (cb < cbAlloced)
    {
        // Realloc the buffer and update all the pointers.
        sigBuffer = (PCOR_SIGNATURE) compiler()->globalHeap.Realloc(sigBuffer, cb);
        sigEnd = sigBuffer + cb;
    }
    return sigBuffer;
}

/*
 * Grows the signature buffer and returns a new sig pointer.
 */
PCOR_SIGNATURE EMITTER::GrowSignature(PCOR_SIGNATURE curSig)
{
    ASSERT(curSig > sigBuffer && curSig <= sigEnd);
    ASSERT(sigBufferInUse);

    int cbEmitted = (int)(curSig - sigBuffer);     // amount emitted.
    int cbAlloced = (int)(sigEnd - sigBuffer);     // ammount allocated.

    // Realloc the buffer and update all the pointers.
    sigBuffer = (PCOR_SIGNATURE) compiler()->globalHeap.Realloc(sigBuffer, cbAlloced + SIGBUFFER_GROW);
    sigEnd = sigBuffer + cbAlloced + SIGBUFFER_GROW;
    curSig = sigBuffer + cbEmitted;
    return curSig;
}


/*
 * Emit a type symbol to the signature. The new current sig pointer is returned.
 * Note that out and ref params are not distinguished in the signatures, but
 * need to be distinguished in parameter properties.
 */
PCOR_SIGNATURE EMITTER::EmitSignatureType(PCOR_SIGNATURE sig, PTYPESYM type)
{
    BYTE b;

    compiler()->symmgr.DeclareType(type);

    for (;;) {
        switch (type->kind) {
        default:
            ASSERT(0);
            return sig;

        case SK_ARRAYSYM:
            if (type->asARRAYSYM()->rank == 1) {
                // Single dimensional array. Emit SZARRAY, element type, size.
                sig = EmitSignatureByte(sig, ELEMENT_TYPE_SZARRAY);
                sig = EmitSignatureType(sig, type->asARRAYSYM()->elementType());
            } else {
                // Known rank > 1
                sig = EmitSignatureByte(sig, ELEMENT_TYPE_ARRAY);
                sig = EmitSignatureType(sig, type->asARRAYSYM()->elementType());
                sig = EmitSignatureUInt(sig, type->asARRAYSYM()->rank);
                sig = EmitSignatureUInt(sig, 0);  // sizes.
                sig = EmitSignatureUInt(sig, type->asARRAYSYM()->rank);  // lower bounds.
                for (int i = 0; i < type->asARRAYSYM()->rank; ++i)
                    sig = EmitSignatureUInt(sig, 0);  // lower bound always zero.
            }
            return sig;

        case SK_VOIDSYM:
            sig = EmitSignatureByte(sig, ELEMENT_TYPE_VOID);
            return sig;

        case SK_PTRSYM:
            sig = EmitSignatureByte(sig, ELEMENT_TYPE_PTR);
            type = type->asPTRSYM()->baseType();
            break;  // continue with base type.

        case SK_PINNEDSYM:
            sig = EmitSignatureByte(sig, ELEMENT_TYPE_PINNED);
            type = type->asPINNEDSYM()->baseType();
            if (type->kind == SK_PTRSYM) {
                sig = EmitSignatureByte(sig, ELEMENT_TYPE_BYREF);
                type = type->asPTRSYM()->baseType();
            }

            break;

        case SK_PARAMMODSYM:
            sig = EmitSignatureByte(sig, ELEMENT_TYPE_BYREF);
            type = type->asPARAMMODSYM()->paramType();
            break;  // continue with base type.

        case SK_AGGSYM:

			if (type == compiler()->symmgr.GetArglistSym()) {
				sig = EmitSignatureByte(sig, ELEMENT_TYPE_SENTINEL);
				return sig;
			} else if (type == compiler()->symmgr.GetNaturalIntSym()) {
				sig = EmitSignatureByte(sig, ELEMENT_TYPE_I);
				return sig;
			}


            // Get the special element type for this class, if any.
            b = compiler()->symmgr.GetElementType(type->asAGGSYM());

            sig = EmitSignatureByte(sig, b);
            if (b == ELEMENT_TYPE_CLASS || b == ELEMENT_TYPE_VALUETYPE) {
                // Emit token for the type ref.
                sig = EmitSignatureToken(sig, GetTypeRef(type));
            }
            return sig;

        case SK_DELETEDTYPESYM:
            // the signature is encoded in the name
            for (ULONG32 i = 0; i < type->asDELETEDTYPESYM()->signatureSize; i += 1) {
                sig = EmitSignatureByte(sig, ((BYTE*)type->name->text)[i]);
            }
            return sig;
        }
    }
}


/*
 * Emit the signature of a member variable. The signature and its size
 * are returned.
 */
PCOR_SIGNATURE EMITTER::SignatureOfMembVar(PMEMBVARSYM sym, int * cbSig)
{
    PCOR_SIGNATURE sig;

    sig = BeginSignature();
    sig = EmitSignatureByte(sig, IMAGE_CEE_CS_CALLCONV_FIELD);
    if (sym->isVolatile) {
        // Mark a volatile with with a required attribute.
        TYPESYM * volatileMod = compiler()->symmgr.GetPredefType(PT_VOLATILEMOD);
        if (volatileMod) { // could be NULL if above call failed.
            sig = EmitSignatureByte(sig, ELEMENT_TYPE_CMOD_REQD);
            sig = EmitSignatureToken(sig, GetTypeRef(volatileMod));
        }
    }
    sig = EmitSignatureType(sig, sym->type);
    return EndSignature(sig, cbSig);
}


/*
 * Emit the signature of a method or property. The signature and its size
 * are returned.
 */
PCOR_SIGNATURE EMITTER::SignatureOfMethodOrProp(PMETHPROPSYM sym, int * cbSig)
{
    if (sym->hasCmodOpt) {
        if (sym->kind == SK_METHSYM && sym->asMETHSYM()->isIfaceImpl) {
            //
            // a compiler generated explicit impl for CMOD_OPT compatibility
            //
            sym = sym->asMETHSYM()->asIFACEIMPLMETHSYM()->explicitImpl->asMETHPROPSYM();
        } else {
            //
            // this only happens when we're overriding an imported method
            // with a CMOD_OPT signature
            //
            AGGSYM *cls = sym->getClass();
            while (sym->getInputFile()->isSource) {
                sym = (METHPROPSYM*) compiler()->clsDeclRec.findHiddenSymbol(sym->name, sym->kind, sym->params, cls->baseClass, cls);
                cls = sym->getClass();
            }
        }
        INFILESYM *infile = sym->getInputFile();
        DWORD scope = sym->getImportScope();

        // get existing signature
        PCCOR_SIGNATURE signature;
        ULONG cbSignature;
        if (sym->kind == SK_METHSYM) {
            TimerStart(TIME_IMI_GETMETHODPROPS);
            CheckHR(infile->metaimport[scope]->GetMethodProps(
	                sym->tokenImport, 		// The method for which to get props.
	                NULL,				// Put method's class here.
	                NULL,	0, NULL,                // Method name
	                NULL,			        // Put flags here.
	                & signature, & cbSignature,	// Method signature
	                NULL,			        // codeRVA
	                NULL));                         // Impl. Flags
            TimerStop(TIME_IMI_GETMETHODPROPS);
        } else {
            TimerStart(TIME_IMI_GETPROPERTYPROPS);
            CheckHR(infile->metaimport[scope]->GetPropertyProps(
	                sym->tokenImport, 		// The method for which to get props.
	                NULL,				// Put method's class here.
	                NULL,	0, NULL,                // Method name
	                NULL,			        // Put flags here.
	                & signature, & cbSignature,	// Method signature
                        NULL, NULL, NULL,               // Default value
                        NULL, NULL,                     // Setter, getter
                        NULL, 0, NULL));                // Other methods
            TimerStop(TIME_IMI_GETPROPERTYPROPS);
        }

        //
        // reserve space for the sig
        //
        BeginSignature();
        PCOR_SIGNATURE translatedSignature = EnsureSignatureSize(cbSignature * 2);

        //
        // convert the imported signature to the current emit scope
        //
        const void *pHash = NULL;
        DWORD cbHash = 0;

        if( infile->assemblyIndex != 0)
            CheckHR(compiler()->linker->GetAssemblyRefHash(infile->mdImpFile, &pHash, &cbHash));
        TimerStart(TIME_IME_TRANSLATESIGWITHSCOPE);
        CheckHR(metaemit->TranslateSigWithScope(
                        infile->assemimport, 
                        pHash, cbHash,            // [IN] Assembly hash value 
                        infile->metaimport[scope],
                        signature,
                        cbSignature,
                        metaassememit,
                        metaemit,
                        translatedSignature,
                        cbSignature * 2, (ULONG*) cbSig));
        TimerStop(TIME_IME_TRANSLATESIGWITHSCOPE);

        return EndSignature(sigBuffer + *cbSig, cbSig);
    }

    PCOR_SIGNATURE sig;
    BYTE callconv;
    int cParams = sym->cParams;
    PTYPESYM retType = sym->retType;

    sig = BeginSignature();

    // Set the calling convention byte.
    callconv = (sym->kind == SK_PROPSYM) ? IMAGE_CEE_CS_CALLCONV_PROPERTY
                                         : (sym->isVarargs ? IMAGE_CEE_CS_CALLCONV_VARARG
                                                           : IMAGE_CEE_CS_CALLCONV_DEFAULT);
    if (! sym->isStatic)
        callconv |= IMAGE_CEE_CS_CALLCONV_HASTHIS;
    sig = EmitSignatureByte(sig, callconv);

    int realParams = cParams;
    if (sym->isVarargs) {
        if (sym->kind == SK_FAKEPROPSYM || sym->kind == SK_FAKEMETHSYM) {
            // this is a ref 
            ASSERT(*(sym->getFParentSym()));
            if ((*(sym->getFParentSym()))->cParams <= cParams) {
                // the original has fewer params (or the same), so the ref must include a sentinel
                realParams--; 
            } else {
                // otherwise the original has more params, so the ref does note have a sentinel...
            }
        } else { // this is the def
            // the def paramcount does not include the sentinel, and its cParam does not count it:
            realParams--;
            cParams--;
        }
    }

    // Emit arg count.
    sig = EmitSignatureUInt(sig, realParams);

    sig = EmitSignatureType(sig, retType);

    // Emit args.
    for (int i = 0; i < cParams; ++i)
        sig = EmitSignatureType(sig, sym->params[i]);

    // And we're done.
    return EndSignature(sig, cbSig);
}

mdToken EMITTER::GetModuleToken()
{
    IMetaDataImport *metaimport= NULL;
    metaemit->QueryInterface(IID_IMetaDataImport, (void**) &metaimport);
    mdModule token;
    CheckHR(metaimport->GetModuleFromScope(&token));
    metaimport->Release();
    return token;
}


/*
 * Emit a signature into a PE and get back a token representing it.
 */
mdToken EMITTER::GetSignatureRef(int cTypes, PTYPESYM * arrTypes)
{
    PCOR_SIGNATURE sig;
    int cbSig;
    TYPESYM ** params;
    mdToken * pToken;

    // Look up in hash table to see if we have seen this signature before.
    // If we have, just return the previously obtained token.
    params = compiler()->symmgr.AllocParams(cTypes, arrTypes, &pToken);

    if (! *pToken) {
        // Create signature for the types.
        sig = BeginSignature();
        sig = EmitSignatureByte(sig, IMAGE_CEE_CS_CALLCONV_LOCAL_SIG);
        sig = EmitSignatureUInt(sig, cTypes);  // Count of types.
        for (int i = 0; i < cTypes; ++i)
            sig = EmitSignatureType(sig, params[i]);
        sig = EndSignature(sig, &cbSig);

        // Define a signature token.
        TimerStart(TIME_IME_GETTOKENFROMSIG);
        CheckHR(metaemit->GetTokenFromSig(sig, cbSig, pToken));
        TimerStop(TIME_IME_GETTOKENFROMSIG);

        RecordEmitToken(pToken);
    }

    return *pToken;
}


/*
 * Get a member ref (or def) for a method for use in emitting code or metadata.
 */
mdToken EMITTER::GetMethodRef(METHSYM *sym)
{
    INFILESYM *inputfile;
    DWORD scope = 0xFFFFFFFF;
    mdToken parent;
    mdMemberRef memberRef;
    PCOR_SIGNATURE sig;
    int cbSig;

#if DEBUG
    if (sym->parent && sym->parent->kind == SK_AGGSYM) {
        // if this fires then the code in FUNCBREC::verifyMethodCall didn't clean up properly...
        ASSERT(!sym->parent->asAGGSYM()->isPrecluded);
    }
#endif

    if (! sym->tokenEmit) {
        // Create a memberRef token for this symbol.

        ASSERT(! sym->isBogus);

        // See if the class come from metadata or source code.
        inputfile = sym->parent->asAGGSYM()->getInputFile();
        scope = sym->parent->asAGGSYM()->getImportScope();
        if (inputfile->isSource || sym->kind == SK_FAKEMETHSYM) {
            ASSERT((inputfile->getOutputFile() != compiler()->curFile->GetOutFile())
                || (sym->kind == SK_FAKEMETHSYM));  // If it's in our file, a def token should already have been assigned.

            // Get parent of the member ref. This is the typeref of the containing class, except for varargs
            // where the parent meth is a methodDef (not a methodRef).
            if (! (sym->kind == SK_FAKEMETHSYM && sym->asFAKEMETHSYM()->parentMethSym &&
                   TypeFromToken(parent = GetMethodRef(sym->asFAKEMETHSYM()->parentMethSym)) == mdtMethodDef))
            {
                parent = GetTypeRef(sym->parent->asAGGSYM(), true);  // COM+ doesn't allow typedef in this case
            }

            // Symbol defined by source code. Define a member ref by name & signature.
            sig = SignatureOfMethodOrProp(sym, &cbSig);

            TimerStart(TIME_IME_DEFINEMEMBERREF);
                CheckHR(metaemit->DefineMemberRef(
                    parent,                             // ClassRef or ClassDef importing a member.
                    sym->name->text,            // member's name
                        sig, cbSig,                     // point to a blob value of COM+ signature
                        &memberRef));
            TimerStop(TIME_IME_DEFINEMEMBERREF);
        }
        else {
            // This symbol was imported from other metadata.
            const void *pHash = NULL;
            DWORD cbHash = 0;

            // First we need the containing class of the method being referenced.
            parent = GetTypeRef(sym->parent->asAGGSYM(), true);  // COM+ doesn't allow typedef in this case

            if( inputfile->assemblyIndex != 0)
                CheckHR(compiler()->linker->GetAssemblyRefHash(inputfile->mdImpFile, &pHash, &cbHash));
            TimerStart(TIME_IME_DEFINEIMPORTMEMBER);
            CheckHR(metaemit->DefineImportMember(             
                inputfile->assemimport,             // [IN] Assembly containing the Member. 
                pHash, cbHash,                      // [IN] Assembly hash value
                inputfile->metaimport[scope],       // [IN] Import scope, with member.  
                sym->tokenImport,                   // [IN] Member in import scope.   
                inputfile->assemimport ? metaassememit : NULL, // [IN] Assembly into which the Member is imported. (NULL if member isn't being imported from an assembly).
                parent,                             // [IN] Classref or classdef in emit scope.    
                &memberRef));                       // [OUT] Put member ref here.   
            TimerStop(TIME_IME_DEFINEIMPORTMEMBER);
        }

        sym->tokenEmit = memberRef;
        RecordEmitToken(& sym->tokenEmit);
    }

    return sym->tokenEmit;
}


/*
 * Get a member ref for a member variable for use in emitting code or metadata.
 */
mdToken EMITTER::GetMembVarRef(PMEMBVARSYM sym)
{
    PINFILESYM inputfile;
    DWORD scope;
    mdToken parent;
    mdMemberRef memberRef;
    PCOR_SIGNATURE sig;
    int cbSig;

    if (! sym->tokenEmit) {
        // Create a memberRef token for this symbol.
        ASSERT(! sym->isBogus);

        // First we need the containing class of the method being referenced.
        parent = GetTypeRef(sym->parent->asAGGSYM(), true);

        // See if the class came from metadata or source code.
        inputfile = sym->parent->asAGGSYM()->getInputFile();
        scope = sym->parent->asAGGSYM()->getImportScope();
        if (inputfile->isSource) {
            ASSERT(inputfile->getOutputFile() != compiler()->curFile->GetOutFile());  // If it's in our file, a def token should already have been assigned.

            // Symbol defined by source code. Define a member ref by name & signature.
            sig = SignatureOfMembVar(sym, & cbSig);

            TimerStart(TIME_IME_DEFINEMEMBERREF);
                CheckHR(metaemit->DefineMemberRef(
                        parent,                         // ClassRef or ClassDef importing a member.
                        sym->name->text,                // member's name
                        sig, cbSig,                     // point to a blob value of COM+ signature
                        &memberRef));
            TimerStop(TIME_IME_DEFINEMEMBERREF);
        }
        else {
            // This symbol was imported from other metadata.

            const void *pHash = NULL;
            DWORD cbHash = 0;

            if( inputfile->assemblyIndex != 0)
                CheckHR(compiler()->linker->GetAssemblyRefHash(inputfile->mdImpFile, &pHash, &cbHash));
            TimerStart(TIME_IME_DEFINEIMPORTMEMBER);
            CheckHR(metaemit->DefineImportMember(             
                inputfile->assemimport,             // [IN] Assembly containing the Member. 
                pHash, cbHash,                      // [IN] Assembly hash value 
                inputfile->metaimport[scope],       // [IN] Import scope, with member.  
                sym->tokenImport,                   // [IN] Member in import scope.   
                inputfile->assemimport ? metaassememit : NULL, // [IN] Assembly into which the Member is imported. (NULL if member isn't being imported from an assembly).
                parent,                             // [IN] Classref or classdef in emit scope.    
                &memberRef));                       // [OUT] Put member ref here.   
            TimerStop(TIME_IME_DEFINEIMPORTMEMBER);
        }

        sym->tokenEmit = memberRef;
        RecordEmitToken(& sym->tokenEmit);
    }

    return sym->tokenEmit;
}


/*
 * Metadata tokens are specific to a particular metadata output file.
 * Once we finish emitting an output file, all the metadata tokens
 * for that output file must be erased. To do that, we record each
 * place that we put a metadata token for the output file into a sym
 * and erase them all at the end. A linked list of 1000 token addresses
 * at a time is used to store these.
 */

/*
 * Remember that a metadata emission token is stored at this address.
 */
void EMITTER::RecordEmitToken(mdToken * tokenAddr)
{
    if (iTokrefCur >= CTOKREF)
    {
        // We need to allocate a new block of addresses.
        TOKREFS * tokrefNew;

        tokrefNew = (TOKREFS *) tokrefHeap.Alloc(sizeof(TOKREFS));
        tokrefNew->next = tokrefList;
        tokrefList = tokrefNew;
        iTokrefCur = 0;
    }

    // Simple case, just remember the address in the current block.
    tokrefList->tokenAddrs[iTokrefCur++] = tokenAddr;
    return;
}

/*
 * Go through all the remembers token addresses and erase them.
 */
void EMITTER::EraseEmitTokens()
{
    TOKREFS * tokref;

    // Go through all the token addresses and free them
    for (tokref = tokrefList; tokref != NULL; tokref = tokref->next)
    {
        // All the blocks are full except the first.
        int cAddr = (tokref == tokrefList) ? iTokrefCur : CTOKREF;

        for (int i = 0; i < cAddr; ++i)
            * tokref->tokenAddrs[i] = 0;  // Erase each token.
    }

    // Free the list of token addresses.
    tokrefHeap.Free( &mark);
    tokrefList = NULL;
    iTokrefCur = CTOKREF;   // Signal that we must allocate a new block right away.
}


/*
 *
 * If this is a local type ref return nil
 * If it is in the same assembly return a ModuleRef
 * If it is in another assembly return an AssemblyRef
 *
 */
mdToken EMITTER::GetScopeForTypeRef(SYM *sym)
{
    INFILESYM *in = sym->getInputFile();
    if (compiler()->curFile->GetOutFile() == in->getOutputFile())
        return TokenFromRid(1, mdtModule);

    if (in->isAddedModule || sym->getAssemblyIndex() != 0) {
        // Either an AddModule or a Reference, either way
        // ALink can do all the dirty work
        if (IsNilToken(in->idLocalAssembly)) {
            CheckHR(compiler()->linker->GetResolutionScope( compiler()->assemID, compiler()->curFile->GetOutFile()->idFile,
                ((sym->getAssemblyIndex() != 0 && in->isSource) ? in->getOutputFile()->idFile : in->mdImpFile),
                &in->idLocalAssembly));
            RecordEmitToken(&in->idLocalAssembly);
        }
        return in->idLocalAssembly; // This could be a module ref or assembly ref
    } else {
        // This is a module we are currently building
        OUTFILESYM *file = in->getOutputFile();
        if (IsNilToken(file->idModRef)) {
            if (!IsNilToken(file->idFile)) {
                // It's already be built,so let ALink do it's job
                CheckHR(compiler()->linker->GetResolutionScope( compiler()->assemID, compiler()->curFile->GetOutFile()->idFile,
                    file->idFile, &file->idModRef));
            } else {
                // We haven't emitted the file yet, so we need to construct our own module-ref
                // Make sure we use the same format as for when we emit the ModuleDef
                CheckHR(metaemit->DefineModuleRef( CPEFile::GetModuleName(file, compiler()), &file->idModRef));
            }
            RecordEmitToken(&file->idModRef);
        }
        return file->idModRef;
    }

    ASSERT(0); // we shouldn't get here
    return mdTokenNil;
}


/*
 * Get a type ref for a type for use in emitting code or metadata.
 * Normally returns either a typeDef or a typeRef, if noDefAllowed is
 * set then only a typeRef is returns (which could be inefficient).
 */
mdToken EMITTER::GetTypeRef(PTYPESYM sym, bool noDefAllowed)
{
    mdToken * tokAddr;
    mdToken tokenTemp;

    if (sym->kind == SK_AGGSYM) {
        tokAddr = & sym->asAGGSYM()->tokenEmit;

        if (noDefAllowed && *tokAddr && TypeFromToken(*tokAddr) == mdtTypeDef)
            tokAddr = & sym->asAGGSYM()->tokenEmitRef;
    }
    else if (sym->kind == SK_ARRAYSYM || sym->kind == SK_PTRSYM)  {
        // We use typespecs instead...

        mdToken * slot;

        if (sym->kind == SK_ARRAYSYM) {
            slot = &(sym->asARRAYSYM()->tokenEmit);
        } else {
            slot = &(sym->asPTRSYM()->tokenEmit);
        }

        if (*slot) {
            return *slot;
        }

        PCOR_SIGNATURE sig;
        int len = 0;

        sig = BeginSignature();
        sig = EmitSignatureType(sig, sym);
        sig = EndSignature(sig, &len);

        TimerStart(TIME_IME_GETTOKENFROMTYPESPEC);
        CheckHR(metaemit->GetTokenFromTypeSpec(
            sig,          // Namespace name.
            len,           // type name
            slot));
        TimerStop(TIME_IME_GETTOKENFROMTYPESPEC);

        RecordEmitToken(slot);

        return *slot;

    } else if (sym->kind == SK_VOIDSYM) {
        // this should be happening in typeof(void) statements only
        return this->GetTypeRef(compiler()->symmgr.GetPredefType(PT_SYSTEMVOID, false), noDefAllowed);
    } else {
        ASSERT(0);  // Can't get a typeref token for any other type...
        return 0;
    }

    // At this point we know:
    //  sym is an aggregate.
    //  tokAddr has the address to assign a token tok.

    if (! *tokAddr) {
        // Create a new typeref token.

#ifdef DEBUG
        // We should never be getting a typeref to a type in the same
        // output file -- they should have typedefs already assigned.
        if (sym->kind == SK_AGGSYM) {
            ASSERT(noDefAllowed || sym->asAGGSYM()->getInputFile()->getOutputFile() != compiler()->curFile->GetOutFile());
        }
#endif //DEBUG

	WCHAR typeNameText[MAX_FULLNAME_SIZE];

	// Get namespace and type name.
	if (METADATAHELPER::GetFullMetadataName(sym, typeNameText, lengthof(typeNameText)))
	{
	    mdToken scope;
	    
        if (sym->parent->kind == SK_NSSYM) 
            scope = (sym->kind == SK_AGGSYM) ? GetScopeForTypeRef(sym) : mdTokenNil;
        else {
            scope = GetTypeRef(sym->parent->asAGGSYM(), true);
            wcscpy(typeNameText, sym->name->text);  // nested types use just the simple name of the type, and the parent class scope.
        }

        ASSERT(!IsNilToken(scope)); // We should NEVER have to emit an unscoped TypeRef!
        
	    TimerStart(TIME_IME_DEFINETYPEREFBYNAME);
	    CheckHR(metaemit->DefineTypeRefByName(
		    scope,                  // A scoped TypeRef
		    typeNameText,           // full type name
		    tokAddr));
	    TimerStop(TIME_IME_DEFINETYPEREFBYNAME);

	    if (tokAddr != &tokenTemp)
		RecordEmitToken(tokAddr);
	}
	else
	{
        // Can't do errorSymbol becuase the symbol name maybe too long and cause astack overflow or something worse
        // so just use the name we've got so far
        size_t textLen = wcslen(typeNameText);
        size_t symLen = wcslen(sym->name->text) + 100; // need room for other error text
        if (lengthof(typeNameText) < (textLen + symLen)) {
            ASSERT(lengthof(typeNameText) > symLen);
            textLen = lengthof(typeNameText) - symLen;
        }
        wcscpy(typeNameText + textLen, L"...");
        wcscpy(typeNameText + textLen + 3, sym->name->text);
        compiler()->clsDeclRec.errorStrFile(sym->getParseTree(), sym->getInputFile(), ERR_ClassNameTooLong, typeNameText);
	    return 0;
	}
}

    return *tokAddr;
}


/*
 * For accessing arrays, the COM+ EE defines four "pseudo-methods" on arrays:
 * constructor, load, store, and load address. This function gets the
 * memberRef for one of these pseudo-methods.
 */
mdToken EMITTER::GetArrayMethodRef(ARRAYSYM *sym, ARRAYMETHOD methodId)
{
    ASSERT(methodId >= 0 && methodId <= ARRAYMETH_COUNT);

    ASSERT(sym->rank > 0);

    ARRAYSYM * methodHolder;

    bool useVarargs = false;
    methodHolder = sym;

    if (! methodHolder->tokenEmitPseudoMethods[methodId]) {
        BYTE flags = useVarargs ? IMAGE_CEE_CS_CALLCONV_VARARG | IMAGE_CEE_CS_CALLCONV_HASTHIS : IMAGE_CEE_CS_CALLCONV_DEFAULT | IMAGE_CEE_CS_CALLCONV_HASTHIS;

        mdToken typeRef = GetTypeRef(sym);
        const WCHAR * name;
        PCOR_SIGNATURE sig;
        int cbSig;
        int i;

        int rank = sym->rank;
        PTYPESYM elementType = sym->elementType();

        sig = BeginSignature();
        sig = EmitSignatureByte(sig, flags);

        // Get the name and signature for the particular pseudo-method.
        switch (methodId)
        {
        case ARRAYMETH_CTOR:

            ASSERT(rank != 0 && !useVarargs);

            name = L".ctor";

            sig = EmitSignatureUInt(sig, rank);
            sig = EmitSignatureByte(sig, ELEMENT_TYPE_VOID);

            // Args are the array sizes.
            for (i = 0; i < rank; ++i)
                sig = EmitSignatureByte(sig, ELEMENT_TYPE_I4);

            break;

        case ARRAYMETH_LOAD:
            name = L"Get";
    
            sig = EmitSignatureUInt(sig, rank);
            sig = EmitSignatureType(sig, elementType);

            if (useVarargs) {
                sig = EmitSignatureByte(sig, ELEMENT_TYPE_SENTINEL);
            }

            // args are the array indices
            for (i = 0; i < rank; ++i)
                sig = EmitSignatureByte(sig, ELEMENT_TYPE_I4);

            break;


        case ARRAYMETH_GETAT:
            name = L"GetAt";

            sig = EmitSignatureUInt(sig, 1);
            sig = EmitSignatureType(sig, elementType);
            sig = EmitSignatureByte(sig, ELEMENT_TYPE_I4);

            break;

        case ARRAYMETH_LOADADDR:
            name = L"Address";

            sig = EmitSignatureUInt(sig, rank);
            sig = EmitSignatureByte(sig, ELEMENT_TYPE_BYREF);
            sig = EmitSignatureType(sig, elementType);
            if (useVarargs) {
                sig = EmitSignatureByte(sig, ELEMENT_TYPE_SENTINEL);
            }

            // args are the array indices
            for (i = 0; i < rank; ++i)
                sig = EmitSignatureByte(sig, ELEMENT_TYPE_I4);

            break;

        case ARRAYMETH_STORE:
            name = L"Set";

            sig = EmitSignatureUInt(sig, rank + 1);
            sig = EmitSignatureByte(sig, ELEMENT_TYPE_VOID);

            if (useVarargs) {
                sig = EmitSignatureByte(sig, ELEMENT_TYPE_SENTINEL);
            }

            // args are the array indices, plus the array element to store.
            for (i = 0; i < rank; ++i)
                sig = EmitSignatureByte(sig, ELEMENT_TYPE_I4);

            sig = EmitSignatureType(sig, elementType);

            break;

        default:
            ASSERT(0);
            return 0;
        }

        sig = EndSignature(sig, &cbSig);

        // Define the member ref for the token.
        TimerStart(TIME_IME_DEFINEMEMBERREF);
        CheckHR(metaemit->DefineMemberRef(
            typeRef,                            // ClassRef or ClassDef importing a member.
            name,                               // member's name
            sig, cbSig,                         // point to a blob value of COM+ signature
            &(methodHolder->tokenEmitPseudoMethods[methodId])));
        TimerStop(TIME_IME_DEFINEMEMBERREF);

        // Remember we created this token.
        RecordEmitToken(&(methodHolder->tokenEmitPseudoMethods[methodId]));
    }

    return methodHolder->tokenEmitPseudoMethods[methodId];
}


/*
 * Emit a constant string to the output file, and get back the
 * token of the string. Indentical strings are folded together.
 */
mdString EMITTER::GetStringRef(const STRCONST * string)
{
    mdString rval;

    TimerStart(TIME_IME_DEFINEUSERSTRING);
    CheckHR(metaemit->DefineUserString(string->text, string->length, &rval));
    TimerStop(TIME_IME_DEFINEUSERSTRING);

    return rval;
}


/*
 * Translate an access level value into flags.
 */
DWORD EMITTER::FlagsFromAccess(ACCESS access)
{
    ASSERT(fdPublic == mdPublic);
    ASSERT(fdPrivate == mdPrivate);
    ASSERT(fdFamily == mdFamily);
    ASSERT(fdAssembly == mdAssem);
    ASSERT(fdFamORAssem == mdFamORAssem);

    switch (access) {
    case ACC_PUBLIC:
        return fdPublic;
    case ACC_PROTECTED:
        return fdFamily;
    case ACC_PRIVATE:
        return fdPrivate;
    case ACC_INTERNAL:
        return fdAssembly;  
    case ACC_INTERNALPROTECTED:
        return fdFamORAssem;
    default:
        ASSERT(0);
        return 0;
    }
}


/*
 * Emit an aggregate type (struct, enum, class, interface) into
 * the metadata. This does not emit any information about members
 * of the aggregrate, but must be done before any aggregate members
 * are emitted.
 */
void EMITTER::EmitAggregateDef(PAGGSYM sym)
{
    DWORD flags;
    WCHAR typeNameText[MAX_FULLNAME_SIZE];
    mdToken tokenBaseClass;
    mdToken * tokenInterfaces;
    int cInterfaces;

    // If this assert triggers, we're emitting the same aggregate twice into an output scope.
    ASSERT(sym->tokenEmit == 0 || TypeFromToken(sym->tokenEmit) == mdtTypeRef);
    ASSERT(sym->isPrepared);

    // Get namespace and type name.
    if (! METADATAHELPER::GetFullMetadataName(sym, typeNameText,  lengthof(typeNameText)))
    {
        // Can't do errorSymbol becuase the symbol name maybe too long and cause a stack overflow or something worse
        // so just use the name we've got so far
        size_t textLen = wcslen(typeNameText);
        size_t symLen = wcslen(sym->name->text) + 100; // need room for other error text
        if (lengthof(typeNameText) < (textLen + symLen)) {
            ASSERT(lengthof(typeNameText) > symLen);
            textLen = lengthof(typeNameText) - symLen;
        }
        wcscpy(typeNameText + textLen, L"...");
        wcscpy(typeNameText + textLen + 3, sym->name->text);
        compiler()->clsDeclRec.errorStrFile(sym->getParseTree(), sym->getInputFile(), ERR_ClassNameTooLong, typeNameText);
        return;
    }

    // Determine flags.
    flags = METADATAHELPER::GetAggregateFlags(sym);

    // Determine base class.
    tokenBaseClass = mdTypeRefNil;
    if (sym->baseClass) {
        tokenBaseClass = GetTypeRef(sym->baseClass);
    }

    // Determine base interfaces.

    // First, count the number of interfaces.
    cInterfaces = 0;
    FOREACHSYMLIST(sym->allIfaceList, listInterfaces)
        ++cInterfaces;
    ENDFOREACHSYMLIST

    // If any interfaces, allocate array and fill it in.
    if (cInterfaces) {
        int i;

        tokenInterfaces = (mdToken *) _alloca(sizeof(mdToken) * (cInterfaces + 1));
        i = 0;
        FOREACHSYMLIST(sym->allIfaceList, listInterfaces)
            tokenInterfaces[i++] = GetTypeRef(listInterfaces->asTYPESYM());
        ENDFOREACHSYMLIST
        tokenInterfaces[i++] = mdTypeRefNil;
    }
    else {
        // No interfaces.
        tokenInterfaces = NULL;
    }

    if (sym->parent->kind == SK_NSSYM) { 
	    // Create the aggregate definition for a top level type.
	    TimerStart(TIME_IME_DEFINETYPEDEF);
	    CheckHR(metaemit->DefineTypeDef(
		        typeNameText,                   // Full name of TypeDef
		        flags,                          // CustomValue flags
		        tokenBaseClass,                 // extends this TypeDef or typeref
		        tokenInterfaces,                // Implements interfaces
		        & sym->tokenEmit));
	    TimerStop(TIME_IME_DEFINETYPEDEF);
    }
    else {
	    // Create the aggregate definition for a nested type.

            ASSERT(sym->parent->asAGGSYM()->isTypeDefEmitted);
	    mdToken tokenParent = GetTypeRef(sym->parent->asAGGSYM());

	    TimerStart(TIME_IME_DEFINENESTEDTYPE);
	    CheckHR(metaemit->DefineNestedType(
                        sym->name->text,                // Simple Name of TypeDef for nested classes.
		        flags,                          // CustomValue flags
		        tokenBaseClass,                 // extends this TypeDef or typeref
		        tokenInterfaces,                // Implements interfaces
		        tokenParent,					// Enclosing class
		        & sym->tokenEmit));
	    TimerStop(TIME_IME_DEFINENESTEDTYPE);
    }

    // record the emit token for later writing to the incremental build file
    if (sym->incrementalInfo) {
        sym->incrementalInfo->token = sym->tokenEmit;
    }

    // Don't emit ComType for TypeDefs in the manifest file
    if (compiler()->BuildAssembly() && sym->hasExternalAccess()) {
        ASSERT(!sym->getInputFile()->canRefThisFile && sym->getInputFile()->isSource);
        HRESULT hr;
        if (sym->parent->kind == SK_NSSYM)
            hr = compiler()->linker->ExportType(compiler()->assemID, sym->getInputFile()->getOutputFile()->idFile,
                sym->tokenEmit, typeNameText, flags, &sym->tokenComType);
        else 
            hr = compiler()->linker->ExportNestedType(compiler()->assemID, sym->getInputFile()->getOutputFile()->idFile,
                sym->tokenEmit, sym->parent->asAGGSYM()->tokenComType, typeNameText, flags, &sym->tokenComType);
        CheckHR(FTL_InternalError, hr);
    }

    RecordEmitToken(& sym->tokenEmit);
}

/*
 * Emit any "special fields" associated with an aggregate. This can't be done
 * in EmitAggregateDef or EmiAggregateInfo due to the special order rules
 * for metadata emitting.
 */
void EMITTER::EmitAggregateSpecialFields(PAGGSYM sym)
{
    if (sym->isEnum) {
        // The underlying type of an enum is represented as a
        // non-static field of that type. Its name is irrelevant, but shouldn't
        // conflict with an identifier.

        PCOR_SIGNATURE sig;
        int cbSig;
        mdToken tokenTemp;

        // Get the signature from the field from enum enum underlying type.
        sig = BeginSignature();
        sig = EmitSignatureByte(sig, IMAGE_CEE_CS_CALLCONV_FIELD);
        sig = EmitSignatureType(sig, sym->underlyingType);
        sig = EndSignature(sig, &cbSig);

        // Create the field definition in the metadata.
        TimerStart(TIME_IME_DEFINEFIELD);
        CheckHR(metaemit->DefineField(
                    sym->tokenEmit,             // Parent TypeDef
                    compiler()->namemgr->GetPredefName(PN_ENUMVALUE)->text, // Name of member
                    fdPublic | fdSpecialName,   // Member attributes
                    sig, cbSig,                 // COM+ signature
                    ELEMENT_TYPE_VOID,          // const type
                    NULL, 0,                    // value of constant
                    & tokenTemp));
        TimerStop(TIME_IME_DEFINEFIELD);
    }
}

/*
 * Emit additional information about an aggregate type (e.g., metadata) into
 * the metadata.
 */
void EMITTER::ReemitAggregateFlags(AGGSYM * sym)
{
    // A type def must already have been assigned via EmitAggregateDef.
    ASSERT(sym->tokenEmit && TypeFromToken(sym->tokenEmit) == mdtTypeDef);

    ASSERT(compiler()->IsIncrementalRebuild());

    // delete existing class layout
    TimerStart(TIME_IME_DELETECLASSLAYOUT);
    HRESULT hr = metaemit->DeleteClassLayout(sym->tokenEmit);
    if (FAILED(hr) && hr != CLDB_E_RECORD_NOTFOUND)
        CheckHR(hr);
    TimerStop(TIME_IME_DELETECLASSLAYOUT);

    TimerStart(TIME_IME_SETTYPEDEFPROPS);
    CheckHR(metaemit->SetTypeDefProps(
        sym->tokenEmit,
        METADATAHELPER::GetAggregateFlags(sym),
        (mdToken) -1,
        NULL));
    TimerStop(TIME_IME_SETTYPEDEFPROPS);

}


/*
 * Give a member variable that has a constant value, the COM+ representation
 * of that constant value is returned as an ELEMENT_TYPE, pointer, and
 * size (in characters, only for strings). tempBuff is an 8-byte temp buffer that is available if desired.
 */
BYTE EMITTER::GetConstValue(PMEMBVARSYM sym, LPVOID tempBuff, LPVOID * ppValue, size_t * pcch)
{
    BYTE elementType;
    PAGGSYM type;
    
    if (sym->type->kind == SK_AGGSYM) {
        type = sym->type->asAGGSYM();
        // Enums use constant types of their element type.
        if (type->isEnum)
            type = type->underlyingType;

        elementType = compiler()->symmgr.GetElementType(type);

    } else if (sym->type->kind == SK_ARRAYSYM) {
        elementType = ELEMENT_TYPE_CLASS;

    } else {
        ASSERT(!"Unknown constant type");
        elementType = ELEMENT_TYPE_VOID;
    }

    *pcch = 0;

    // This code needs to support little endian and big endian
    // The metaemit api's will store in the appropriate format
    switch (elementType) {
    case ELEMENT_TYPE_STRING:
        if (! sym->constVal.strVal) {
            // The "null" string (not the empty string).
            elementType = ELEMENT_TYPE_CLASS;
            *ppValue = NULL;
        }
        else {
            // Return length and text of the string.
            *ppValue = sym->constVal.strVal->text;
            *pcch = sym->constVal.strVal->length;
            break;
        }
        break;

    case ELEMENT_TYPE_BOOLEAN:
    case ELEMENT_TYPE_I1:
        *ppValue = tempBuff;
        *(__int8 *)tempBuff = (__int8)sym->constVal.iVal;
        break;

    case ELEMENT_TYPE_U1:
        *ppValue = tempBuff;
        *(unsigned __int8 *)tempBuff = (unsigned __int8)sym->constVal.uiVal;
        break;

    case ELEMENT_TYPE_U2:
    case ELEMENT_TYPE_CHAR:
        *ppValue = tempBuff;
        *(UINT16 *)tempBuff = (UINT16)sym->constVal.uiVal;
        break;

    case ELEMENT_TYPE_I2: 
        *ppValue = tempBuff;
        *(INT16 *)tempBuff = (INT16)sym->constVal.iVal;
        break;

    case ELEMENT_TYPE_I4:
    case ELEMENT_TYPE_U4:
        ASSERT(sizeof(sym->constVal) == 4);
        *ppValue = &sym->constVal;
        break;

    case ELEMENT_TYPE_I8: 
    case ELEMENT_TYPE_U8:
        *ppValue = sym->constVal.longVal;
        break;

    case ELEMENT_TYPE_R4:
        *ppValue = tempBuff;
        *(float *)tempBuff = (float) *sym->constVal.doubleVal;
        break;


    case ELEMENT_TYPE_R8:
        *ppValue = sym->constVal.doubleVal;
        break;

    case ELEMENT_TYPE_OBJECT:
        // element type Object does not work, use:
        elementType = ELEMENT_TYPE_CLASS;
        // Fall Through
    case ELEMENT_TYPE_CLASS:
        // Only valid constant of class type is NULL.
        *ppValue = NULL; 
        break;

    default:
        ASSERT(0);      // This shouldn't happen.
        elementType = ELEMENT_TYPE_VOID;
        *ppValue = NULL;
        break;
    }

    return elementType;
}

/*
 * given a constval structure associated with a given type, return a variant that has the same stuff.
 * returns true on success, false on failure. v is assume uninitialized.
 * This does not need to be in little endian but should be in native endian format
 */
bool EMITTER::VariantFromConstVal(TYPESYM * type, CONSTVAL * cv, VARIANT * v)
{
    if (type->kind != SK_AGGSYM)
        return false;

    if (type->asAGGSYM()->isEnum)
        type = type->asAGGSYM()->underlyingType;

    switch (compiler()->symmgr.GetElementType(type->asAGGSYM())) {
    case ELEMENT_TYPE_U1: 
        V_VT(v) = VT_UI1;
        V_UI2(v) = (unsigned char)cv->iVal;
        return true;

    case ELEMENT_TYPE_I1:
        V_VT(v) = VT_I1;
        V_I1(v) = (signed char)cv->iVal;
        return true;

    case ELEMENT_TYPE_U2: 
    case ELEMENT_TYPE_CHAR:
        V_VT(v) = VT_UI2;
        V_UI2(v) = (unsigned short)cv->iVal;
        return true;

    case ELEMENT_TYPE_I2: 
        V_VT(v) = VT_I2;
        V_I2(v) = (signed short)cv->iVal;
        return true;

    case ELEMENT_TYPE_I4:
        V_VT(v) = VT_I4;
        V_I4(v) = cv->iVal;
        return true;

    case ELEMENT_TYPE_U4:
        V_VT(v) = VT_UI4;
        V_UI4(v) = cv->uiVal;
        return true;

    case ELEMENT_TYPE_I8: 
        V_VT(v) = VT_I8;
        V_I8(v) = * (cv->longVal);
        return true;

    case ELEMENT_TYPE_U8:
        V_VT(v) = VT_UI8;
        V_UI8(v) = *(cv->ulongVal);
        return true;

    case ELEMENT_TYPE_BOOLEAN:
        V_VT(v) = VT_BOOL;
        V_BOOL(v) = (cv->iVal) ? VARIANT_TRUE : VARIANT_FALSE;
        return true;

    case ELEMENT_TYPE_R4:
        V_VT(v) = VT_R4;
        V_R4(v) = (float) *cv->doubleVal;
        return true;

    case ELEMENT_TYPE_R8:
        V_VT(v) = VT_R8;
        V_R8(v) = *cv->doubleVal; 
        return true;

    case ELEMENT_TYPE_CLASS:
    case ELEMENT_TYPE_OBJECT:
        // Only valid constant of class type is NULL.
        break;

    case ELEMENT_TYPE_VALUETYPE:
        if (type == compiler()->symmgr.GetPredefType(PT_DECIMAL, false)) {
            V_DECIMAL(v) = *cv->decVal;
            V_VT(v) = VT_DECIMAL;
            return true;
        }
        break;

    case ELEMENT_TYPE_STRING:
        if (! cv->strVal) {
            // The "null" string (not the empty string).
            break;
        }
        else {
            // Return length and text of the string.
            V_VT(v) = VT_BSTR;
            V_BSTR(v) = SysAllocStringLen(cv->strVal->text, cv->strVal->length);
            return true;
        }
        break;

    }

    return false;
}


/*
 * getthe flags for a field
 */
DWORD EMITTER::GetMembVarFlags(MEMBVARSYM *sym)
{
    DWORD flags;
    // Determine the flags.
    flags = FlagsFromAccess(sym->access);
    if (sym->isStatic)
        flags |= fdStatic;
    if (sym->isConst) {
        if (sym->type->isPredefType(PT_DECIMAL)) {
            // COM+ doesn;t support decimal constants
            // they are initialized with field initializers in the static constructor
            flags |= (fdStatic | fdInitOnly);
        } else {
            flags |= (fdStatic | fdLiteral);
        }
    }
    else if (sym->isReadOnly)
        flags |= fdInitOnly;

    return flags;
}

/*
 * Emitted field def into the metadata. The parent aggregate must
 * already have been emitted into the current metadata output file.
 */
void EMITTER::EmitMembVarDef(PMEMBVARSYM sym)
{
    mdTypeDef parentToken;      // Token of the parent aggregrate.
    PCCOR_SIGNATURE sig;
    int cbSig;
    BYTE tempBuff[8];
    DWORD constType = ELEMENT_TYPE_VOID;
    void * constValue = NULL;
    size_t cchConstValue = 0;

    // Get typedef token for the containing class/enum/struct. This must
    // be present because the containing class must have been emitted already.
    parentToken = sym->parent->asAGGSYM()->tokenEmit;
    ASSERT(TypeFromToken(parentToken) == mdtTypeDef && parentToken != mdTypeDefNil);

    // Determine the flags.
    if (sym->isConst) {
        if (! sym->type->isPredefType(PT_DECIMAL)) {

            // Get the value of the constant.
            constType = GetConstValue(sym, tempBuff, &constValue, &cchConstValue);
        }
    }

    // Get the signature from the field from its type.
    sig = SignatureOfMembVar(sym, & cbSig);

    // Create the field definition in the metadata.
    TimerStart(TIME_IME_DEFINEFIELD);
    CheckHR(metaemit->DefineField(
                parentToken,            // Parent TypeDef
                sym->name->text,        // Name of member
                GetMembVarFlags(sym),   // Member attributes
                sig, cbSig,             // COM+ signature
                constType,              // const type Flag
                constValue, (ULONG)cchConstValue,  // value of constant
                & sym->tokenEmit));
    TimerStop(TIME_IME_DEFINEFIELD);

    RecordEmitToken(& sym->tokenEmit);
}

/*
 * Allocate the metadata token for a parameter
 */
void EMITTER::DefineParam(mdToken tokenMethProp, int index, mdToken *paramToken)
{
    ASSERT(*paramToken == mdTokenNil);

    TimerStart(TIME_IME_DEFINEPARAM);
    CheckHR(metaemit->DefineParam(
            tokenMethProp,                      // Owning method/property
            index,                              // Which param
            NULL,                               // Param name
            0,                                  // param flags
            0,                                  // C++ Type Flag
            NULL, 0,                            // Default value
            paramToken));                       // [OUT] Put param token here
    TimerStop(TIME_IME_DEFINEPARAM);
}

/*
 * Set or Reset the parameter properties for a method.
 * The parameter properties is only the parameter name.
 * The parameter mode (in, out, in/out) are set by PCAs
 * index is the parameter index (0 == return value, 1 is first parameter)
 * NOTE: all parameters must be named
 */
void EMITTER::EmitParamProp(mdToken tokenMethProp, int index, TYPESYM *type, PARAMINFO *paramInfo)
{
    ASSERT((index == 0 && !paramInfo->name) || (index != 0 && paramInfo->name));

    PNAME name;

    // Get parameter name.
    name = paramInfo->name;

    // NOTE: param flags are done through Pseudo-CAs which come after ParamProps
    ASSERT(!paramInfo->isIn && !paramInfo->isOut);

    // Param mode.
    if (compiler()->options.m_fNOCODEGEN)
        return;

    // Set param properties if not the default.
    if (name)
    {
        if (mdTokenNil == paramInfo->tokenEmit) {
            TimerStart(TIME_IME_DEFINEPARAM);
            CheckHR(metaemit->DefineParam(
                    tokenMethProp,                      // Owning method/property
                    index,                              // Which param
                    name->text,                         // Param name
                    0,                                  // param flags
                    0,                                  // C++ Type flags
                    NULL, 0,                            // Default value
                    &paramInfo->tokenEmit));            // [OUT] Put param token here
            TimerStop(TIME_IME_DEFINEPARAM);
        } else {
            TimerStart(TIME_IME_SETPARAMPROPS);
            CheckHR(metaemit->SetParamProps(
                    paramInfo->tokenEmit,
                    name->text,                         // Param name
                    0,                                  // param flags
                    0,                                  // C++ type flags
                    NULL, 0));                          // Default value
            TimerStop(TIME_IME_SETPARAMPROPS);
        }
    } else if (mdTokenNil != paramInfo->tokenEmit && compiler()->IsIncrementalRebuild()) {
        TimerStart(TIME_IME_SETPARAMPROPS);
        CheckHR(metaemit->SetParamProps(
                paramInfo->tokenEmit,
                NULL,                               // Param name
                0,                                  // param flags
                0,                                  // C++ type flags
                NULL, 0));                          // Default value
        TimerStop(TIME_IME_SETPARAMPROPS);
    }
}

/*
 * Returns the flags for a method
 */
DWORD EMITTER::GetMethodFlags(METHSYM *sym)
{
    DWORD flags;

    // Determine the flags.
    flags = FlagsFromAccess(sym->access);

    if (!sym->isHideByName) {
        flags |= mdHideBySig;
    }
    if (sym->isStatic) {
        flags |= mdStatic;
    }
    if (sym->isCtor) {
        flags |= mdSpecialName;
    }

    if (sym->isVirtual) {
        ASSERT(! sym->isCtor);
        flags |= mdVirtual;
    }
    else if (sym->isMetadataVirtual) {
        // Non-virtual in the language, but be virtual in the metadata. Also emit 
        // mdFinal so we read it in as non-virtual/sealed.
        flags |= mdVirtual | mdFinal;
    }

    if (sym->isVirtual || sym->isMetadataVirtual) {
        if (sym->isOverride && !sym->requiresMethodImplForOverride) {
            flags |= mdReuseSlot;
        } else {
            flags |= mdNewSlot;
        }
    }

    if (sym->isAbstract) {
        flags |= mdAbstract;
    }

    if (sym->isOperator || sym->isPropertyAccessor || sym->isEventAccessor) {
        flags |= mdSpecialName;
    }
    if (sym->explicitImpl) {
        ASSERT(sym->isMetadataVirtual);
    }

    return flags;
}

mdToken EMITTER::GetGlobalFieldDef(METHSYM * sym, unsigned int count, unsigned int size, BYTE **pBuffer)
{
    mdToken token = GetGlobalFieldDef(sym, count, NULL, size);

    ULONG rva, offset;
    *pBuffer = (BYTE*) compiler()->AllocateRVABlock(sym->getInputFile(), size, 8, &rva, &offset);
    
    TimerStart(TIME_IME_SETFIELDRVA);
    CheckHR(metaemit->SetFieldRVA(token, rva));
    TimerStop(TIME_IME_SETFIELDRVA);

    return token;
}

mdToken EMITTER::GetGlobalFieldDef(METHSYM * sym, unsigned int count, TYPESYM * type, unsigned int size)
{

	ASSERT(type || size);

    if (0 == sym->getInputFile()->getOutputFile()->globalTypeToken)
    {
        if (compiler()->IsIncrementalRebuild())
        {
            // See if we already have one
            CComPtr<IMetaDataImport> metaimport;
            CheckHR(metaemit->QueryInterface(IID_IMetaDataImport, (void**)&metaimport));
            if (metaimport) {
                HRESULT hr = S_OK;
                TimerStart(TIME_INCBUILD);
                hr = metaimport->FindTypeDefByName(
                        compiler()->namemgr->GetPredefName(PN_GLOBALFIELD)->text,   // Full name
                        mdTokenNil,
                        & sym->getInputFile()->getOutputFile()->globalTypeToken);
	            TimerStop(TIME_INCBUILD);
                if (FAILED(hr) && hr != CLDB_E_RECORD_NOTFOUND)
                    CheckHR(hr);
            }
        }
        
        if (0 == sym->getInputFile()->getOutputFile()->globalTypeToken)
        {
            TimerStart(TIME_IME_DEFINETYPEDEF);
            CheckHR(metaemit->DefineTypeDef(
                    compiler()->namemgr->GetPredefName(PN_GLOBALFIELD)->text,               // Full name of TypeDef
                    tdClass | tdNotPublic,          // CustomValue flags
                    GetTypeRef(compiler()->symmgr.GetPredefType(PT_OBJECT)),   // extends this TypeDef or typeref
                    NULL,                           // Implements interfaces
                    & sym->getInputFile()->getOutputFile()->globalTypeToken));
            TimerStop(TIME_IME_DEFINETYPEDEF);
        }
        RecordEmitToken(& sym->getInputFile()->getOutputFile()->globalTypeToken);
    }

    WCHAR bufferW[255];
    mdToken dummyToken = mdTokenNil;
    if (!type) {
        ASSERT(size);
        swprintf(bufferW, L"$$struct%#x-%d", sym->tokenEmit, count);

        TimerStart(TIME_IME_DEFINENESTEDTYPE);
        CheckHR(metaemit->DefineNestedType(
            bufferW,		                // Simple Name of TypeDef for nested classes.
            tdExplicitLayout | tdNestedPrivate | tdSealed,     // CustomValue flags
            GetTypeRef(compiler()->symmgr.GetPredefType(PT_VALUE)),   // extends this TypeDef or typeref
            NULL,                // Implements interfaces
            sym->getInputFile()->getOutputFile()->globalTypeToken,
            & dummyToken));
        TimerStop(TIME_IME_DEFINENESTEDTYPE);

        TimerStart(TIME_IME_SETCLASSLAYOUT);
        CheckHR(metaemit->SetClassLayout(
            dummyToken,
            1,
            NULL,
            size));
        TimerStop(TIME_IME_SETCLASSLAYOUT);

        RecordGlobalToken(dummyToken, sym->getInputFile());
    }

    swprintf(bufferW, L"$$method%#x-%d", sym->tokenEmit, count);

    PCOR_SIGNATURE sig;
    int cbSig;

    mdToken tokenTemp;

    sig = BeginSignature();
    sig = EmitSignatureByte(sig, IMAGE_CEE_CS_CALLCONV_FIELD);
    if (dummyToken != mdTokenNil) {
        sig = EmitSignatureByte(sig, ELEMENT_TYPE_VALUETYPE);
        sig = EmitSignatureToken(sig, dummyToken);
    } else {
        sig = EmitSignatureType(sig, type);
    }
    sig = EndSignature(sig, &cbSig);
    
    TimerStart(TIME_IME_DEFINEFIELD);
    CheckHR(metaemit->DefineField(
                sym->getInputFile()->getOutputFile()->globalTypeToken,             // Parent TypeDef
                bufferW, // Name of member
                fdAssembly | fdStatic | fdPrivateScope,  // Member attributes
                sig, cbSig,                 // COM+ signature
                ELEMENT_TYPE_VOID,          // const type
                NULL, 0,                    // value of constant
                & tokenTemp));
    TimerStop(TIME_IME_DEFINEFIELD);
    RecordGlobalToken(tokenTemp, sym->getInputFile());

    return tokenTemp; 
}

/*
 * Remember that a a global field token was used for this INFILESYM.
 */
void EMITTER::RecordGlobalToken(mdToken token, INFILESYM *infile)
{
    // Don't waste time doing this if we don't need an incremental file
    if (compiler()->options.m_fINCBUILD)
    {
        ULONG index = infile->cGlobalFields++ & (lengthof(infile->globalFields->pTokens) - 1);
        if (index == 0)
        {
            // We need to allocate a new block of addresses.
            INFILESYM::TOKENLIST * toklistNew;

            toklistNew = (INFILESYM::TOKENLIST *) compiler()->globalSymAlloc.AllocZero(sizeof(INFILESYM::TOKENLIST));
            toklistNew->pNext = infile->globalFields;
            infile->globalFields = toklistNew;
        }

        // Simple case, just remember the token in the current block.
        infile->globalFields->pTokens[index] = token;
    }
    return;
}

/*
 * Emit the methoddef for a method into the metadata.
 */
void EMITTER::EmitMethodDef(METHSYM * sym)
{
    mdTypeDef parentToken;      // Token of the parent aggregrate.
    DWORD flags;
    PCCOR_SIGNATURE sig;
    int cbSig;
    const WCHAR * nameText;
    WCHAR nameBuffer[MAX_FULLNAME_SIZE];

    // Get typedef token for the containing class/enum/struct. This must
    // be present because the containing class must have been emitted already.
    parentToken = sym->parent->asAGGSYM()->tokenEmit;
    ASSERT(TypeFromToken(parentToken) == mdtTypeDef && parentToken != mdTypeDefNil);

    // Set "nameText" to the output name.
    if (sym->name == NULL) {
        // Explicit method implementations don't have a name in the language. Synthesize 
        // a name -- the name has "." characters in it so it can't possibly collide.
        METADATAHELPER::GetExplicitImplName(sym, nameBuffer, lengthof(nameBuffer));
        nameText = nameBuffer;
    }
    else {
        nameText = sym->name->text;
    }

    // Determine the flags.
    flags = GetMethodFlags(sym);

    // Get signature of method.
    sig = SignatureOfMethodOrProp(sym, &cbSig);

    // Define the method.
    TimerStart(TIME_IME_DEFINEMETHOD);
    CheckHR(metaemit->DefineMethod(
                parentToken,        // Parent TypeDef
                nameText,           // name of member
                flags,              // Member attributes
                sig, cbSig,         // point to a blob value of COM+ signature
                0,                  // RVA of method code (will be set in EmitMethodInfo)
                0,                  // implementation flags (will be set in EmitMethodInfo)
                & sym->tokenEmit));
    TimerStop(TIME_IME_DEFINEMETHOD);

    RecordEmitToken(& sym->tokenEmit);

    EmitMethodImpl(sym);
}

/*
 * Emit a method code and additional method information into the metadata.
 *
 * cbCode indicates the amount of code space this method needs, 0 if no code.
 * if aligned is true, 4 byte alignment is used, else 1 byte alignment.
 *
 * Returns a pointer that points to memory to store the code into.
 */
void EMITTER::EmitMethodInfo(METHSYM * sym, METHINFO * info)
{
    DWORD implFlags;


    // A method def must already have been assigned via EmitMethodDef.
    ASSERT(compiler()->options.m_fNOCODEGEN || (sym->tokenEmit && TypeFromToken(sym->tokenEmit) == mdtMethodDef));

    //
    // set impl flags
    //
    if (info->isMagicImpl) {
        if (sym->isSysNative) {
            // COM classic coclass constructor
            ASSERT(sym->isCtor && !sym->isStatic);

            implFlags = miManaged | miRuntime | miInternalCall;
        } else {
            // Magic method with implementation supplied by run-time.
            // delegate construcotr and Invoke
            ASSERT(sym->getClass()->isDelegate);

            implFlags = miManaged | miRuntime;
        }
    } else if (sym->isAbstract) {
        implFlags = 0;
    } else {
        implFlags = miManaged | miIL;       // Our code is always managed IL.
    }

    if (info->isSynchronized)
        implFlags |= miSynchronized;

    if (compiler()->options.m_fNOCODEGEN)
        return;

    // Set the impl flags.
    TimerStart(TIME_IME_SETMETHODIMPLFLAGS);
    CheckHR(metaemit->SetMethodImplFlags(sym->tokenEmit, implFlags));
    TimerStop(TIME_IME_SETMETHODIMPLFLAGS);
}

//
// rewrites the flags for a method
//
void EMITTER::ResetMethodFlags(METHSYM *sym)
{
    ASSERT(sym->tokenEmit != 0);

    TimerStart(TIME_IME_SETMETHODPROPS);
    CheckHR(metaemit->SetMethodProps(sym->tokenEmit, GetMethodFlags(sym), UINT_MAX, UINT_MAX));
    TimerStop(TIME_IME_SETMETHODPROPS);
}

void EMITTER::SetEntryPoint(METHSYM *sym)
{
    // Normal methods don't have methodImpl set. Explicit interface method implementations do.
    if (! sym->explicitImpl) {
        // Normal method
        POUTFILESYM outfile = sym->getInputFile()->getOutputFile();

        // Is this an entry point for the program? We only allow one.
        if (! outfile->isDll) {
            if (outfile->entrySym != NULL) {
                if (!outfile->multiEntryReported) {
                    // If multiple entry points, we want to report all the duplicate entry points, including
                    // the first one we found.
                    compiler()->clsDeclRec.errorSymbolStr(outfile->entrySym, ERR_MultipleEntryPoints, outfile->name->text);
                    outfile->multiEntryReported = true;
                }

                compiler()->clsDeclRec.errorSymbolStr(sym, ERR_MultipleEntryPoints, outfile->name->text);
            }

            outfile->entrySym = sym;

            if (outfile->isUnnamed()) {
                compiler()->symmgr.SetOutFileName(sym->getInputFile());
                outfile->hasDefaultName = true;
            }
        }
    }
}

// If an output file has a "/main" option
// find the class specified and then find the Main method
// report errors as needed
void EMITTER::FindEntryPoint(OUTFILESYM *outfile)
{
    if (outfile->entryClassName == NULL)
        return;

    OUTFILESYM *other;
    SYM *ns = compiler()->clsDeclRec.findSymName (outfile->entryClassName);

    if (!ns) {
        compiler()->Error(NULL, ERR_MainClassNotFound, outfile->entryClassName);
        return;
    } else if (ns->kind != SK_AGGSYM || ns->asAGGSYM()->isEnum || ns->asAGGSYM()->isInterface) {
        if (ns->kind == SK_NSSYM) // We can pass a NSSYM to errorSym, so get a NSDECLSYM
            ns = ns->asNSSYM()->firstDeclaration();
        compiler()->clsDeclRec.errorSymbol(ns, ERR_MainClassNotClass);
        return;
    } else if ((other = ns->getInputFile()->getOutputFile()) != outfile) {
        if (ns->getInputFile()->isSource) {
            compiler()->clsDeclRec.errorSymbol(ns, ERR_MainClassWrongFile);
        } else
            if (outfile->isUnnamed()) {
                compiler()->symmgr.SetOutFileName(outfile->firstInfile());
                outfile->hasDefaultName = true;
            }
            compiler()->Error(NULL, ERR_MainClassIsImport, outfile->entryClassName, outfile->name->text);
        return;
    }

    FindEntryPointInClass(ns->asPARENTSYM());
}

void EMITTER::FindEntryPointInClass(PARENTSYM *parent)
{
    OUTFILESYM *outfile = parent->getInputFile()->getOutputFile();

    // We've found the specified class, now let's look for a Main
    SYM *sym, *next;
    METHSYM *meth = NULL;

    sym = compiler()->symmgr.LookupGlobalSym(compiler()->namemgr->GetPredefName(PN_MAIN), parent, MASK_METHSYM);
    if (sym)
        meth = sym->asMETHSYM();

    while (meth != NULL) {
        // Must be public, static, void/int Main ()
        // with no args or String[] args
         // If you change this code also change the code in CLSDREC::defineMethod (it does basically the same thing)
       if (meth->isStatic &&
            !meth->isPropertyAccessor &&
            (meth->retType == compiler()->symmgr.GetVoid() || meth->retType->isPredefType(PT_INT)) &&
                 (meth->cParams == 0 ||
                   (meth->cParams == 1 && meth->params[0]->kind == SK_ARRAYSYM &&
                    meth->params[0]->asARRAYSYM()->elementType()->isPredefType(PT_STRING))))
            {
                SetEntryPoint(meth);
            }
        next = meth->nextSameName;
        meth = NULL;
        while (next != NULL) {
            if (next->kind == SK_METHSYM) {
                meth = next->asMETHSYM();
                break;
            }
            next = next->nextSameName;
        }
    }

    if (outfile->entrySym == NULL) {
        compiler()->clsDeclRec.errorSymbol(parent, ERR_NoMainInClass);
        if (sym) {
            meth = sym->asMETHSYM();

            while (meth != NULL) {
                // Report anything that looks like Main ()
                if (!meth->isPropertyAccessor)
                {
                    compiler()->clsDeclRec.errorSymbol(sym, WRN_InvalidMainSig);
                }

                next = meth->nextSameName;
                meth = NULL;
                while (next != NULL) {
                    if (next->kind == SK_METHSYM) {
                        meth = next->asMETHSYM();
                        break;
                    }
                    next = next->nextSameName;
                }
            }
        }
    }
}


void *EMITTER::EmitMethodRVA(PMETHSYM sym, ULONG cbCode, ULONG alignment)
{
    ULONG codeRVA, offset;
    void * codeLocation;

    // Only call this if you are really emitting code
    ASSERT(cbCode > 0);

    // A method def must already have been assigned via EmitMethodDef.
    ASSERT(sym->tokenEmit &&
           (TypeFromToken(sym->tokenEmit) == mdtMethodDef));

    codeLocation = compiler()->AllocateRVABlock(sym->getInputFile(), cbCode, alignment, &codeRVA, &offset);

    // Set the code location.
    TimerStart(TIME_IME_SETRVA);
    CheckHR(metaemit->SetRVA(sym->tokenEmit, codeRVA));
    TimerStop(TIME_IME_SETRVA);

    // Return the block of code allocated for this method.
    return codeLocation;
}

/*
 * helper for creating blocks of unicode XML text
 */
class XMLGenerator
{
public:

#define ALLOCSIZE 256

    XMLGenerator(NRHEAP *heap) :
        heap(heap)
    {
        heap->Mark(&mark);
        begin   = (LPWSTR) heap->Alloc(ALLOCSIZE * sizeof(WCHAR));
        current = begin;
        max     = begin + ALLOCSIZE;
    }
    ~XMLGenerator()
    {
        heap->Free(&mark);
    }

    // accessors
    LPWSTR      getXML()    { return begin; }
    size_t      getSize()   { return (size_t)(current - begin) * sizeof(WCHAR); }

    // mutators
    void        clear()     { current = begin; }
    void        emitTag(LPCWSTR tag)
    {
        expand(2 + wcslen(tag));

        addChar(L'<');
        addString(tag, wcslen(tag));
        addChar(L'>');
    }
    void    emitString(LPCWSTR str, size_t length)
    {
        expand(length);

        addString(str, length);
    }

private:

    void        addChar(WCHAR ch)
    {
        ASSERT(current < max);
        *current++ = ch;
    }

    void        addString(LPCWSTR str, size_t length)
    {
        ASSERT((current + length) < max);
        memcpy(current, str, length * sizeof(WCHAR));
        current += length;
    }

    void        expand(size_t size)
    {
        if (max < (current + size + 1))
        {
            size_t allocSize = ((current + size + 1 - max) + (ALLOCSIZE - 1)) & ~(ALLOCSIZE-1);
            heap->Alloc(allocSize * sizeof(WCHAR));
            max += allocSize;
        }
    }

    NRHEAP *    heap;
    NRMARK      mark;
    LPWSTR      begin;
    LPWSTR      current;
    LPWSTR      max;

#undef ALLOCSIZE
};

/*
 * Emit a "namespace" directive into the debug info
 * for this namespace
 */
void EMITTER::EmitDebugNamespace(NSSYM * ns)
{
    WCHAR buffer[MAX_FULLNAME_SIZE];

    if (METADATAHELPER::GetFullMetadataName(ns, buffer, lengthof(buffer)))
    {
        CheckHRDbg(debugemit->UsingNamespace(buffer));
    }
}

/*
 * Begin emitting debug information for a method.
 */
void EMITTER::EmitDebugMethodInfoStart(METHSYM * sym)
{
    TimerStart(TIME_ISW_OPENMETHOD);
    CheckHRDbg(debugemit->OpenMethod(sym->tokenEmit));
    TimerStop(TIME_ISW_OPENMETHOD);

    // The using statements MUST belong to a scope
    // So open the default scope
    EmitDebugScopeStart(0);

    // Add all the using statements
    NSSYM * myNS = NULL;
    PARENTSYM * psym;

    for (psym = sym->containingDeclaration();
         psym != NULL;
         psym = psym->containingDeclaration())
    {
        if (psym->kind != SK_NSDECLSYM)
            continue;

        if (!myNS)
            myNS = psym->asNSDECLSYM()->namespaceSymbol; // record my namespace

        // Emit all the using clauses in this ns declaration.
        if (psym->asNSDECLSYM()->usingClausesResolved) 
        {
            FOREACHSYMLIST(psym->asNSDECLSYM()->usingClauses, usingSym)
                if (usingSym->kind == SK_NSSYM)
                    EmitDebugNamespace(usingSym->asNSSYM());
            ENDFOREACHSYMLIST
        }
    }

    // We implicit use our own namespace and all psym namespaces.
    NSSYM * root = compiler()->symmgr.GetRootNS();
    for (psym = myNS; psym != root && psym != NULL; psym = psym->parent)
    {
        if (psym->kind != SK_NSSYM)
            continue;

        EmitDebugNamespace(psym->asNSSYM());
    }
}

/*
 * Stop emitting debug information for a method.
 */
void EMITTER::EmitDebugMethodInfoStop(METHSYM * sym, int ilOffsetEnd)
{
    // Close the default scope
    EmitDebugScopeEnd(ilOffsetEnd);

    TimerStart(TIME_ISW_CLOSEMETHOD);
    CheckHRDbg(debugemit->CloseMethod());
    TimerStop(TIME_ISW_CLOSEMETHOD);
}

/*
 *  Emit a block of debug info (line/col , il offset pairs) associated
 *  with the given method
 */
void EMITTER::EmitDebugBlock(METHSYM * sym, int count, unsigned int * offsets, unsigned int * lines, unsigned int * cols, unsigned int * endLines, unsigned int * endCols)
{
    ASSERT(debugemit);

    if (count == 0)
        return;

    int newCount = 0;
    // Create an array for the files
    INFILESYM ** files;
    INFILESYM * infile = sym->getInputFile();
    CSourceModule * pModule = infile->pData->GetModule();
    bool useMap = !!pModule->hasMap();
    
    // Arrays for each of the constiuent files
    unsigned int * newOffsets;
    unsigned int * newLines;  
    unsigned int * newCols;   
    unsigned int * newEndLines;  
    unsigned int * newEndCols;   

    if (useMap) {
        files = (INFILESYM**)compiler()->GetStandardHeap()->Alloc(sizeof(INFILESYM*)*count);
        newOffsets = (unsigned int*)compiler()->GetStandardHeap()->Alloc(sizeof(unsigned int)*count);
        newLines = (unsigned int*)compiler()->GetStandardHeap()->Alloc(sizeof(unsigned int)*count);
        newCols = (unsigned int*)compiler()->GetStandardHeap()->Alloc(sizeof(unsigned int)*count);
        newEndLines = (unsigned int*)compiler()->GetStandardHeap()->Alloc(sizeof(unsigned int)*count);
        newEndCols = (unsigned int*)compiler()->GetStandardHeap()->Alloc(sizeof(unsigned int)*count);

        for (int i = 0; i < count; i++) {
            NAME * Filename = NULL;
            NAME * endFile = NULL;
            if (lines[i] == NO_DEBUG_LINE) {
                ASSERT(endLines[i] == NO_DEBUG_LINE);
                endLines[i] = NO_DEBUG_LINE;
                files[i] = infile;
                continue;
            }

            pModule->MapLocation((long*) &lines[i], &Filename);
            pModule->MapLocation((long*) &endLines[i], &endFile);
            if (endFile != Filename) {
                Filename = endFile;
            }
            if (!Filename) {
                files[i] = infile;
                continue;
            }
            INFILESYM* sym = compiler()->symmgr.LookupGlobalSym( Filename, infile->getOutputFile(), MASK_INFILESYM)->asINFILESYM();
            if (!sym) {
                sym = compiler()->symmgr.LookupGlobalSym( Filename, infile->getOutputFile(), MASK_SYNTHINFILESYM)->asANYINFILESYM();
            }
            if (!sym) {
                files[i] = compiler()->symmgr.CreateSynthSourceFile( Filename->text, infile->getOutputFile());
            } else {
                files[i] = sym;
            }
        }
    } else {
        files = (INFILESYM**)compiler()->GetStandardHeap()->Alloc(sizeof(INFILESYM*));
        files[0] = infile;
        newOffsets = offsets;
        newLines = lines;
        newCols = cols;
        newEndLines = endLines;
        newEndCols = endCols;
        newCount = 0;
    }


    while (newCount >= 0) {
        infile = files[newCount];
        if (infile->documentWriter == NULL) {
            // No ISymDocumentWriter interface yet obtained for this input file.
            ICSSourceText * pText = NULL;
            PCWSTR name;

            if (infile->pData) {
                CheckHR(infile->pData->GetModule()->GetSourceText(&pText));
                CheckHR(pText->GetName(&name));
                pText->Release();
            } else {
                name = infile->name->text;
            }

            if (compiler()->options.debugNoPath) {
                LPWSTR baseName = wcsrchr(name, L'\\');
                if (baseName) {
                    name = baseName + 1;
                }
            }

            TimerStart(TIME_ISW_DEFINEDOCUMENT);
            CheckHRDbg(debugemit->DefineDocument(
                (WCHAR *) name,
                (GUID *) & CorSym_LanguageType_CSharp,
                (GUID *) & CorSym_LanguageVendor_Microsoft,
                (GUID *) & CorSym_DocumentType_Text,
                &(infile->documentWriter)));
            TimerStop(TIME_ISW_DEFINEDOCUMENT);
        }

        int j = 0;
        if (useMap) {
            int i = newCount;
            newCount = -1;
            for (; i < count; i++) {
                if (files[i] == infile) {
                    newOffsets[j] = offsets[i];
                    newLines[j] = lines[i] + 1;  // make them 1 based, now that they are mapped
                    newEndLines[j] = endLines[i] + 1;
                    newCols[j] = cols[i];
                    newEndCols[j] = endCols[i];

					// PDB limitation: Can't be <0 or > 62 lines in a statement.
					if (newEndLines[j] - newLines[j] > 62 || newEndLines[j] - newLines[j] < 0)
					{
						newEndLines[j] = newLines[j]; newEndCols[j] = newCols[j];
					}

                    j++;
                    files[i] = NULL;
                } else if (newCount == -1 && files[i] != NULL) {
                    newCount = i;
                }
            }
        } else {
            j = count;
            newCount = -1;
            for (int i = 0; i < j; i++) {
                newLines[i]++;	// make them 1 based (since we aren't mapping them)
                newEndLines[i]++;

				// PDB limitation: Can't be <0 or > 62 lines in a statement.
				if (newEndLines[i] - newLines[i] > 62 || newEndLines[i] - newLines[i] < 0)
				{
					newEndLines[i] = newLines[i]; newEndCols[i] = newCols[i];
				}
            }
        }

        TimerStart(TIME_ISW_DEFINESEQUENCEPOINTS);
        CheckHRDbg(debugemit->DefineSequencePoints(
            infile->documentWriter,
            j,
            newOffsets,
            newLines,
            newCols,
            newEndLines,
            newEndCols));
        TimerStop(TIME_ISW_DEFINESEQUENCEPOINTS);
    }

    compiler()->GetStandardHeap()->Free(files);
    if (useMap) {
        compiler()->GetStandardHeap()->Free(newOffsets);
        compiler()->GetStandardHeap()->Free(newLines);
        compiler()->GetStandardHeap()->Free(newCols);
        compiler()->GetStandardHeap()->Free(newEndLines);
        compiler()->GetStandardHeap()->Free(newEndCols);
    }
}

/*
 * Emit a local variable into a given debug scope
 */
void EMITTER::EmitDebugTemporary(TYPESYM * type, LPCWSTR name, unsigned slot)
{
    PCOR_SIGNATURE sig;
    int cbSig;

    // Create signature for the type
    sig = BeginSignature();
    sig = EmitSignatureType(sig, type);
    sig = EndSignature(sig, &cbSig);

    TimerStart(TIME_ISW_DEFINELOCALVAR);
    CheckHRDbg(debugemit->DefineLocalVariable(
        (WCHAR *) name,
        VAR_IS_COMP_GEN,
        cbSig,
        (BYTE *) sig,
        ADDR_IL_OFFSET,
        slot, 0, 0,
        0, 0));
    TimerStop(TIME_ISW_DEFINELOCALVAR);
}


/*
 * Emit a local variable into a given debug scope
 */
void EMITTER::EmitDebugLocal(LOCVARSYM * sym, int ilOffsetStart, int ilOffsetEnd)
{
    PCOR_SIGNATURE sig;
    int cbSig;

    // Create signature for the type
    sig = BeginSignature();
    sig = EmitSignatureType(sig, sym->type);
    sig = EndSignature(sig, &cbSig);

    TimerStart(TIME_ISW_DEFINELOCALVAR);
    CheckHRDbg(debugemit->DefineLocalVariable(
        (WCHAR *) sym->name->text,
        0,
        cbSig,
        (BYTE *) sig,
        ADDR_IL_OFFSET,
        sym->slot.ilSlot, 0, 0,
        ilOffsetStart, (ilOffsetEnd != 0) ? ilOffsetEnd - 1 : 0));  // ilOffsetEnd is inclusive, not exclusive as passed in.
    TimerStop(TIME_ISW_DEFINELOCALVAR);
}

/*
 * Emit a local constant into a given debug scope
 */
void EMITTER::EmitDebugLocalConst(LOCVARSYM * sym)
{
    PCOR_SIGNATURE sig;
    int cbSig;
    VARIANT value;

    // Create signature for the type
    sig = BeginSignature();
    sig = EmitSignatureType(sig, sym->type);
    sig = EndSignature(sig, &cbSig);

    VariantInit(&value);

    if (VariantFromConstVal(sym->type, & sym->constVal, & value)) {
        TimerStart(TIME_ISW_DEFINECONSTANT);
        CheckHRDbg(debugemit->DefineConstant(
            sym->name->text,
            value,
            cbSig,
            (BYTE *) sig));
        TimerStop(TIME_ISW_DEFINECONSTANT);
    }

    VariantClear(&value);
}

/*
 * Emit a scope for local variables starting at the given IL offset
 */
void EMITTER::EmitDebugScopeStart(int ilOffsetStart)
{
    unsigned int dummy;

    TimerStart(TIME_ISW_OPENSCOPE);
    CheckHRDbg(debugemit->OpenScope(ilOffsetStart, &dummy));
    TimerStop(TIME_ISW_OPENSCOPE);
}

/*
 * Emit a scope for local variables ending at the given IL offset (the passed
 * offset is the next IL instruction.
 */
void EMITTER::EmitDebugScopeEnd(int ilOffsetEnd)
{
    TimerStart(TIME_ISW_CLOSESCOPE);
    // The CloseScope API is apparently inclusive, while we're passing in an exclusive address.
    CheckHRDbg(debugemit->CloseScope(ilOffsetEnd - 1));
    TimerStop(TIME_ISW_CLOSESCOPE);
}


/*
 * Returns the flags to be set for a property.
 */
DWORD EMITTER::GetPropertyFlags(PROPSYM *sym)
{
    DWORD flags = 0;

    return flags;
}


/*
 * Emit a property into the metadata. The parent aggregate, and the
 * property accessors, must already have been emitted into the current
 * metadata output file.
 */
void EMITTER::EmitPropertyDef(PPROPSYM sym)
{
    // don't emit property which are explicit interface implementations
    // just do their accessors
    if (sym->explicitImpl) {
        return;
    }

    mdTypeDef parentToken;          // Token of the parent aggregrate.
    mdMethodDef getToken, setToken; // Tokens of getter and setter accessors.
    PCCOR_SIGNATURE sig;
    int cbSig;

    // Get typedef token for the containing class/enum/struct. This must
    // be present because the containing class must have been emitted already.
    parentToken = sym->parent->asAGGSYM()->tokenEmit;
    ASSERT(TypeFromToken(parentToken) == mdtTypeDef && parentToken != mdTypeDefNil);

    // Determine getter/setter methoddef tokens.
    getToken = setToken = mdMethodDefNil;
    if (sym->methGet) {
        getToken = sym->methGet->tokenEmit;
        ASSERT((!sym->name || TypeFromToken(getToken) == mdtMethodDef) && getToken != mdMethodDefNil);
    }
    if (sym->methSet) {
        setToken = sym->methSet->tokenEmit;
        ASSERT((!sym->name || TypeFromToken(setToken) == mdtMethodDef) && setToken != mdMethodDefNil);
    }

    // Get signature of property.
    sig = SignatureOfMethodOrProp(sym, &cbSig);

    // Define the propertydef.
    TimerStart(TIME_IME_DEFINEPROPERTY);
    CheckHR(metaemit->DefineProperty(
                parentToken,                    // the class/interface on which the property is being defined
                sym->getRealName()->text,       // name of property
                GetPropertyFlags(sym),          // property attributes (CorPropertyAttr)
                sig, cbSig,                     // blob value of COM+ signature
                0,                              // C++ type flags
                NULL, 0,                        // Constant/default value
                setToken, getToken,             // Getter and setter accessors
                NULL,                           // Other accessors
                & sym->tokenEmit));
    TimerStop(TIME_IME_DEFINEPROPERTY);

    RecordEmitToken(& sym->tokenEmit);
}

/*
 * Returns the flags to be set for an event.
 */
DWORD EMITTER::GetEventFlags(EVENTSYM *sym)
{
    DWORD flags = 0;

    return flags;
}

/*
 * Emit an event into the metadata. The parent aggregate, and the
 * event accessors, and the event field/property must already have been 
 * emitted into the current metadata output file.
 */
void EMITTER::EmitEventDef(PEVENTSYM sym)
{
    mdTypeDef parentToken;             // Token of parent aggregate
    mdToken typeToken;                 // Token for type of the event (a delegate type).
    mdMethodDef addToken, removeToken; // Token of add/remove accessors

    // If an explicit implementation, don't emit.
    if (sym->getExplicitImpl())
        return;

    // Get typedef token for the containing class/enum/struct. This must
    // be present because the containing class must have been emitted already.
    parentToken = sym->parent->asAGGSYM()->tokenEmit;
    ASSERT(TypeFromToken(parentToken) == mdtTypeDef && parentToken != mdTypeDefNil);

    // Get typeref token for the delegate type of this event.
    typeToken = GetTypeRef(sym->type);

    // Determine adder/remover methoddef tokens.
    addToken = sym->methAdd->tokenEmit;
    ASSERT((!sym->name || TypeFromToken(addToken) == mdtMethodDef) && addToken != mdMethodDefNil);
    removeToken = sym->methRemove->tokenEmit;
    ASSERT((!sym->name || TypeFromToken(removeToken) == mdtMethodDef) && removeToken != mdMethodDefNil);

    // Define the eventdef.
    TimerStart(TIME_IME_DEFINEEVENT);
    CheckHR(metaemit->DefineEvent(    
        parentToken,                        // the class/interface on which the event is being defined 
        sym->name->text,                    // Name of the event   
        GetEventFlags(sym),                 // Event attributes (CorEventAttr)
        typeToken,                          // a reference (mdTypeRef or mdTypeRef) to the Event class 
        addToken,                           // required add method 
        removeToken,                        // required remove method  
        mdMethodDefNil,                     // optional fire method    
        NULL,                               // optional array of other methods associate with the event    
        &sym->tokenEmit));                  //  output event token 
    TimerStop(TIME_IME_DEFINEEVENT);

    RecordEmitToken(& sym->tokenEmit);
}


/* Handle an error on a Define Attribute call. */
void EMITTER::HandleAttributeError(HRESULT hr, BASENODE *parseTree, INFILESYM *infile, METHSYM *method)
{
    CComPtr<IErrorInfo> errorInfo;
    WCHAR * descString;

    // Try to get a really good error string with GetErrorInfo. Otherwise, use the HRESULT to get one.
    // ErrHR does both.
    descString = compiler()->ErrHR(hr, true);

    if (parseTree && infile) {
		compiler()->clsDeclRec.errorStrStrFile(parseTree, infile, ERR_CustomAttributeError, descString, compiler()->ErrSym(method->getClass()));
    } else {
        // we either have both a file and NODE* or neither
        ASSERT(!parseTree && !infile);
        compiler()->Error( NULL, ERR_CustomAttributeError, descString, compiler()->ErrSym(method->getClass()));
    }
}
 
/*
 * Emit a user defined custom attribute.
 */
void EMITTER::EmitCustomAttribute(BASENODE *parseTree, INFILESYM *infile, mdToken token, METHSYM *method, BYTE *buffer, unsigned bufferSize)
{
    mdToken ctorToken = GetMethodRef(method);
    ASSERT(ctorToken != mdTokenNil);
    HRESULT hr = S_OK;
    
    if (token == mdtAssembly) {
        AGGSYM *cls = method->parent->asAGGSYM();
        TimerStart(TIME_IME_DEFINECUSTOMATTRIBUTE);
        hr = compiler()->linker->EmitAssemblyCustomAttribute(compiler()->assemID, compiler()->curFile->GetOutFile()->idFile,
            ctorToken, buffer, bufferSize, cls->isSecurityAttribute, cls->isMultipleAttribute);
        TimerStop(TIME_IME_DEFINECUSTOMATTRIBUTE);
    } else if (method->getClass()->isSecurityAttribute) {
        // Security attributes must be accumulated for later emitting.
        SaveSecurityAttribute(token, ctorToken, method, buffer, bufferSize);
    } else {
        TimerStart(TIME_IME_DEFINECUSTOMATTRIBUTE);
        hr = metaemit->DefineCustomAttribute(token, ctorToken, buffer, bufferSize, NULL);
        TimerStop(TIME_IME_DEFINECUSTOMATTRIBUTE);
    }

    if (FAILED(hr)) {
        HandleAttributeError(hr, parseTree, infile, method);
    }
}

/*
 * Save a security attribute on a symbol for later emitted (all security attributes for a 
 * given symbol must be emitted in one call).
 */
void EMITTER::SaveSecurityAttribute(mdToken token, mdToken ctorToken, METHSYM * method, BYTE * buffer, unsigned bufferSize)
{
    SECATTRSAVE * pSecAttrSave;
    ASSERT(token == tokenSecAttrSave || cSecAttrSave == 0);

    // Allocate new structure to save the information.
    pSecAttrSave = (SECATTRSAVE *) compiler()->globalHeap.Alloc(sizeof(SECATTRSAVE));
    pSecAttrSave->next = listSecAttrSave;
    pSecAttrSave->ctorToken = ctorToken;
    pSecAttrSave->method = method;
    pSecAttrSave->bufferSize = bufferSize;
    pSecAttrSave->buffer = (BYTE *) compiler()->globalHeap.Alloc(bufferSize);
    memcpy(pSecAttrSave->buffer, buffer, bufferSize);

    // Link into the global list of security attributes.
    listSecAttrSave = pSecAttrSave;
    tokenSecAttrSave = token;
    ++cSecAttrSave;
}

/*
 * Emit pending security attributes on a symbol.
 */
void EMITTER::EmitSecurityAttributes(BASENODE *parseTree, INFILESYM *infile, mdToken token)
{
    if (cSecAttrSave == 0)
        return;  // nothing to do.

    ASSERT(token == tokenSecAttrSave);

    COR_SECATTR * rSecAttrs = (COR_SECATTR *) _alloca(cSecAttrSave * sizeof(COR_SECATTR));
    SECATTRSAVE * pSecAttrSave;
    int i;

    // Put all the saved attributes into one array.
    pSecAttrSave = listSecAttrSave;
    i = 0;
    while (pSecAttrSave) {
        rSecAttrs[i].tkCtor = pSecAttrSave->ctorToken;
        rSecAttrs[i].pCustomAttribute = pSecAttrSave->buffer;
        rSecAttrs[i].cbCustomAttribute = pSecAttrSave->bufferSize;
        ++i;
        pSecAttrSave = pSecAttrSave->next;
    }

    HRESULT hr;
    TimerStart(TIME_IME_DEFINESECURITYATTRIBUTESET);
    hr = metaemit->DefineSecurityAttributeSet(token, rSecAttrs, cSecAttrSave, NULL);
    TimerStop(TIME_IME_DEFINESECURITYATTRIBUTESET);
    if (FAILED(hr)) {
        HandleAttributeError(hr, parseTree, infile, listSecAttrSave->method); // use first attribute for error reporting.
    }

    FreeSavedSecurityAttributes();
}

void EMITTER::FreeSavedSecurityAttributes()
{
    SECATTRSAVE * pSecAttrSave;

    // Free the saved attrbutes.
    pSecAttrSave = listSecAttrSave;
    while (pSecAttrSave) {
        SECATTRSAVE *pNext;
        pNext = pSecAttrSave->next;
        compiler()->globalHeap.Free(pSecAttrSave->buffer);
        compiler()->globalHeap.Free(pSecAttrSave);
        pSecAttrSave = pNext;
    }

    cSecAttrSave = 0;
    listSecAttrSave = NULL;
}

void EMITTER::DeletePermissions(INFILESYM *inputfile, mdToken token)
{
    ASSERT(TypeFromToken(token) == mdtTypeDef || TypeFromToken(token) == mdtMethodDef || TypeFromToken(token) == mdtAssembly);

    mdPermission permsBuffer[4];
    ULONG cPerms, iPerm;
    HCORENUM corenum = 0;

    for (DWORD scope = 0; scope < inputfile->dwScopes; scope++) {
        IMetaDataImport * metaimport = inputfile->metaimport[scope];

        do {
            // Get next batch of attributes.
            TimerStart(TIME_IMI_ENUMPERMISSIONSETS);
            CheckHR(metaimport->EnumPermissionSets(&corenum, token, 0, permsBuffer, lengthof(permsBuffer), &cPerms));
            TimerStop(TIME_IMI_ENUMPERMISSIONSETS);

            // Process each attribute.
            for (iPerm = 0; iPerm < cPerms; ++iPerm) {
                DeleteToken(permsBuffer[iPerm]);
            }
        } while (cPerms > 0);

        metaimport->CloseEnum(corenum);
    }
}

void EMITTER::DeleteRelatedParamTokens(INFILESYM *inputfile, mdToken token)
{
    if (mdTokenNil == token) return;

    DeleteCustomAttributes(inputfile, token);

    // delete possibly existing field marshal
    TimerStart(TIME_IME_DELETEFIELDMARSHAL);
    HRESULT hr = metaemit->DeleteFieldMarshal(token);
    if (FAILED(hr) && hr != CLDB_E_RECORD_NOTFOUND)
        CheckHR(hr);
    TimerStop(TIME_IME_DELETEFIELDMARSHAL);
}

void EMITTER::DeleteRelatedMethodTokens(INFILESYM *inputfile, mdToken token)
{
    if (mdTokenNil == token) return;

    DeleteCustomAttributes(inputfile, token);
    DeletePermissions(inputfile, token);

    // delete existing PInvoke map
    TimerStart(TIME_IME_DELETEPINVOKEMAP);
    HRESULT hr = metaemit->DeletePinvokeMap(token);
    if (FAILED(hr) && hr != CLDB_E_RECORD_NOTFOUND)
        CheckHR(hr);
    TimerStop(TIME_IME_DELETEPINVOKEMAP);
}

void EMITTER::DeleteRelatedTypeTokens(INFILESYM *inputfile, mdToken token)
{
    if (mdTokenNil == token) return;

    DeleteCustomAttributes(inputfile, token);
    DeletePermissions(inputfile, token);

    // delete existing class layout
    TimerStart(TIME_IME_DELETECLASSLAYOUT);
    HRESULT hr = metaemit->DeleteClassLayout(token);
    if (FAILED(hr) && hr != CLDB_E_RECORD_NOTFOUND)
        CheckHR(hr);
    TimerStop(TIME_IME_DELETECLASSLAYOUT);

}

void EMITTER::DeleteRelatedFieldTokens(INFILESYM *inputfile, mdToken token)
{
    if (mdTokenNil == token) return;

    DeleteCustomAttributes(inputfile, token);

    // delete possibly existing field marshal
    TimerStart(TIME_IME_DELETEFIELDMARSHAL);
    HRESULT hr = metaemit->DeleteFieldMarshal(token);
    if (FAILED(hr) && hr != CLDB_E_RECORD_NOTFOUND)
        CheckHR(hr);
    TimerStop(TIME_IME_DELETEFIELDMARSHAL);
}

void EMITTER::DeleteRelatedAssemblyTokens(INFILESYM *inputfile, mdToken token)
{
    if (mdTokenNil == token) return;

    DeleteCustomAttributes(inputfile, token);

    DeletePermissions(inputfile, token);
}

/*
 * deletes all the custom attributes on a token
 */
void EMITTER::DeleteCustomAttributes(INFILESYM *inputfile, mdToken token)
{
    if (mdTokenNil == token) return;

    HCORENUM corenum;           // For enumerating tokens.
    mdToken attributesBuffer[32];
    ULONG cAttributes, iAttribute;
    corenum = 0;

    for (DWORD scope = 0; scope < inputfile->dwScopes; scope++) {
        IMetaDataImport * metaimport = inputfile->metaimport[scope];

        do {
            // Get next batch of attributes.
            TimerStart(TIME_IMI_ENUMCUSTOMATTRIBUTES);
            CheckHR(metaimport->EnumCustomAttributes(&corenum, token, 0, attributesBuffer, lengthof(attributesBuffer), &cAttributes));
            TimerStop(TIME_IMI_ENUMCUSTOMATTRIBUTES);

            // Process each attribute.
            for (iAttribute = 0; iAttribute < cAttributes; ++iAttribute) {
                DeleteToken(attributesBuffer[iAttribute]);
            }
        } while (cAttributes > 0);

        metaimport->CloseEnum(corenum);
    }
}


/*
 * Record the fact that a metadata token lives at the offset within
 * the last returned code block. This is used as a fix-up notification.
 */
void EMITTER::DefineTokenLoc(int offset)
{
    // Since we now should never move tokens, this isn't necessary.
    //CheckHR(ceefilegen->AddSectionReloc(ilSection, lastMethodLoc + offset, 0, srRelocMapToken));
}


#ifdef DEBUG
// We implement IMapToken in debug just to make sure the metadata engine
// never wants to remap a token.
// QueryInterface - we don't implement.
STDMETHODIMP EMITTER::QueryInterface(REFIID riid,void __RPC_FAR *__RPC_FAR *ppObj)
{
    *ppObj = NULL;

    if (riid == IID_IUnknown || riid == IID_IMapToken)
    {
        *ppObj = (IMapToken *)this;
        return S_OK;
    }

    return E_NOINTERFACE;
}

// Add a reference.
STDMETHODIMP_(ULONG) EMITTER::AddRef()
{
    return 1;
}

// Release a reference.
STDMETHODIMP_(ULONG) EMITTER::Release()
{
    return 1;
}

// Map a token.
STDMETHODIMP EMITTER::Map(mdToken tkImp, mdToken tkEmit)
{
    // This should never get called!
    ASSERT(0);
    return E_FAIL;
}

#endif //DEBUG

void EMITTER::ReemitMembVar(MEMBVARSYM *sym)
{
    ASSERT(sym->tokenEmit);

    DeleteRelatedFieldTokens(sym->getInputFile(), sym->tokenEmit);

    BYTE tempBuff[8];
    DWORD constType = ELEMENT_TYPE_VOID;
    void * constValue = NULL;
    size_t cchConstValue = 0;

    // Determine the flags.
    if (sym->isConst) {
        if (! sym->type->isPredefType(PT_DECIMAL)) {

            // Get the value of the constant.
            constType = GetConstValue(sym, tempBuff, &constValue, &cchConstValue);

        }
    }

    // Create the field definition in the metadata.
    TimerStart(TIME_IME_SETFIELDPROPS);
    CheckHR(metaemit->SetFieldProps(
                sym->tokenEmit,
                GetMembVarFlags(sym),
                constType,
                constValue,
                (ULONG)cchConstValue));
    TimerStop(TIME_IME_SETFIELDPROPS);
}

void EMITTER::ReemitMethod(METHSYM *sym)
{
    ASSERT(sym->tokenEmit);

    DeleteRelatedMethodTokens(sym->getInputFile(), sym->tokenEmit);

    // Define the method.
    ResetMethodFlags(sym);

    // don't need to reemit method impl flags
    // that'll be done later by EmitMethodInfo

}

void EMITTER::EmitMethodImpl(PMETHSYM sym)
{
    // Normal methods don't have methodImpl set. Explicit interface method implementations do.
    if (sym->explicitImpl) {
        TimerStart(TIME_IME_DEFINEMETHODIMPL);
        HRESULT hr = metaemit->DefineMethodImpl(
                    sym->getClass()->tokenEmit,     // The class implementing the method
                    sym->tokenEmit,                 // our methoddef token
                    GetMethodRef(sym->explicitImpl->asMETHSYM()));  // method being implemented
        if (!compiler()->IsIncrementalRebuild() || hr != META_S_DUPLICATE)
        {
            CheckHR(hr);
        }
        TimerStop(TIME_IME_DEFINEMETHODIMPL);
    }
}

void EMITTER::ReemitProperty(PROPSYM *sym)
{
    // don't emit property which are explicit interface implementations
    // just do their accessors
    ASSERT(!sym->explicitImpl);

    DeleteCustomAttributes(sym->getInputFile(), sym->tokenEmit);

    mdMethodDef getToken, setToken; // Tokens of getter and setter accessors.

    // Determine getter/setter methoddef tokens.
    getToken = setToken = mdMethodDefNil;
    if (sym->methGet) {
        getToken = sym->methGet->tokenEmit;
        ASSERT((!sym->name || TypeFromToken(getToken) == mdtMethodDef) && getToken != mdMethodDefNil);
    }
    if (sym->methSet) {
        setToken = sym->methSet->tokenEmit;
        ASSERT((!sym->name || TypeFromToken(setToken) == mdtMethodDef) && setToken != mdMethodDefNil);
    }

    // Define the propertydef.
    TimerStart(TIME_IME_SETPROPERTYPROPS);
    CheckHR(metaemit->SetPropertyProps(
                sym->tokenEmit,                 // the property token
                GetPropertyFlags(sym),          // property attributes (CorPropertyAttr)
                0,                              // C++ type flags
                NULL, 0,                        // Constant/default value
                setToken, getToken,             // Getter and setter accessors
                NULL));                         // Other accessors
    TimerStop(TIME_IME_SETPROPERTYPROPS);
}

void EMITTER::ReemitEvent(EVENTSYM *sym)
{
    mdToken typeToken;                 // Token for type of the event (a delegate type).
    mdMethodDef addToken, removeToken; // Token of add/remove accessors

    // If an explicit implementation, don't emit.
    ASSERT(!(sym->implementation && sym->implementation->kind == SK_PROPSYM && sym->implementation->asPROPSYM()->explicitImpl));

    DeleteCustomAttributes(sym->getInputFile(), sym->tokenEmit);

    // Get typeref token for the delegate type of this event.
    typeToken = GetTypeRef(sym->type);

    // Determine adder/remover methoddef tokens.
    addToken = sym->methAdd->tokenEmit;
    ASSERT((!sym->name || TypeFromToken(addToken) == mdtMethodDef) && addToken != mdMethodDefNil);
    removeToken = sym->methRemove->tokenEmit;
    ASSERT((!sym->name || TypeFromToken(removeToken) == mdtMethodDef) && removeToken != mdMethodDefNil);

    // Define the eventdef.
    TimerStart(TIME_IME_SETEVENTPROPS);
    CheckHR(metaemit->SetEventProps(
        sym->tokenEmit,                     // the class/interface on which the event is being defined 
        GetEventFlags(sym),                 // Event attributes (CorEventAttr)
        typeToken,                          // a reference (mdTypeRef or mdTypeRef) to the Event class 
        addToken,                           // required add method 
        removeToken,                        // required remove method  
        mdMethodDefNil,                     // optional fire method    
        NULL));                             // optional array of other methods associate with the event    
    TimerStop(TIME_IME_SETEVENTPROPS);
}

void EMITTER::DeleteToken(mdToken token)
{
    TimerStart(TIME_IME_DELETETOKEN);
    CheckHR(metaemit->DeleteToken(token));
    TimerStop(TIME_IME_DELETETOKEN);
}
