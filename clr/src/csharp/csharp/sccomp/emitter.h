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
// File: emitter.h
//
// Defines the structures used to emit COM+ metadata and create executable files.
// ===========================================================================

#include <iceefilegen.h>

#define CTOKREF 1000   // Number of token refs per block. Fits nicely in a page.

// structure for saving security attributes.
struct SECATTRSAVE {
    SECATTRSAVE * next;
    mdToken ctorToken;
    METHSYM * method;
    BYTE * buffer;
    unsigned bufferSize;
};

/*
 *  The class that handles emitted PE files, and generating the metadata
 *  database within the files.
 */
class EMITTER
#ifdef DEBUG
// We implement this just for debug purposes.
: IMapToken
#endif //DEBUG
{
public:
    EMITTER();
    ~EMITTER();
    void Term();

    COMPILER * compiler();

    bool BeginOutputFile();
    bool EndOutputFile(bool writeFile);

    void SetEntryPoint(METHSYM *sym);
    void FindEntryPoint(OUTFILESYM *outfile);
    void FindEntryPointInClass(PARENTSYM *parent);

    void EmitAggregateDef(PAGGSYM sym);
    void EmitAggregateSpecialFields(PAGGSYM sym);
    void EmitMembVarDef(PMEMBVARSYM sym);
    void EmitPropertyDef(PPROPSYM sym);
    void EmitEventDef(PEVENTSYM sym);
    void EmitMethodDef(PMETHSYM sym);
    void EmitMethodInfo(PMETHSYM sym, PMETHINFO info);
    void EmitMethodImpl(PMETHSYM sym);
    void DefineParam(mdToken tokenMethProp, int index, mdToken *paramToken);
    void EmitParamProp(mdToken tokenMethProp, int index, TYPESYM *type, PARAMINFO *paramInfo);
    void *EmitMethodRVA(PMETHSYM sym, ULONG cbCode, ULONG alignment);
    void ResetMethodFlags(METHSYM *sym);
    DWORD GetMethodFlags(METHSYM *sym);
    DWORD GetMembVarFlags(MEMBVARSYM *sym);
    DWORD GetPropertyFlags(PROPSYM *sym);
    DWORD GetEventFlags(EVENTSYM *sym);
    void EmitDebugMethodInfoStart(METHSYM * sym);
    void EmitDebugMethodInfoStop(METHSYM * sym, int ilOffsetEnd);
    void EmitDebugBlock(METHSYM * sym, int count, unsigned int * offsets, unsigned int * lines, unsigned int * cols, unsigned int * endLines, unsigned int * endCols);
    void EmitDebugTemporary(TYPESYM * type, LPCWSTR name, unsigned slot);
    void EmitDebugLocal(LOCVARSYM * sym, int ilOffsetStart, int ilOffsetEnd);
    void EmitDebugLocalConst(LOCVARSYM * sym);
    void EmitDebugScopeStart(int ilOffsetStart);
    void EmitDebugScopeEnd(int ilOffsetEnd);
    void EmitCustomAttribute(BASENODE *parseTree, INFILESYM *infile, mdToken token, METHSYM *method, BYTE *buffer, unsigned bufferSize);
    bool HasSecurityAttributes() { return cSecAttrSave > 0; }
    void EmitSecurityAttributes(BASENODE *parseTree, INFILESYM *infile, mdToken token);

    void ReemitAggregateFlags(PAGGSYM sym);
    void ReemitMembVar(MEMBVARSYM *sym);
    void ReemitMethod(METHSYM *sym);
    void ReemitProperty(PROPSYM *sym);
    void ReemitEvent(EVENTSYM *sym);
    void DeleteToken(mdToken token);
    void DeleteCustomAttributes(INFILESYM *inputfile, mdToken token);
    void DeletePermissions(INFILESYM *inputfile, mdToken token);
    void DeleteRelatedParamTokens(INFILESYM *inputfile, mdToken token);
    void DeleteRelatedMethodTokens(INFILESYM *inputfile, mdToken token);
    void DeleteRelatedTypeTokens(INFILESYM *inputfile, mdToken token);
    void DeleteRelatedFieldTokens(INFILESYM *inputfile, mdToken token);
    void DeleteRelatedAssemblyTokens(INFILESYM *inputfile, mdToken token);

    mdToken GetMethodRef(PMETHSYM sym);
    mdToken GetMembVarRef(PMEMBVARSYM sym);
    mdToken GetTypeRef(PTYPESYM sym, bool noDefAllowed = false);
    mdToken GetArrayMethodRef(PARRAYSYM sym, ARRAYMETHOD methodId);
    mdToken GetSignatureRef(int cTypes, PTYPESYM * arrTypes);
    mdString GetStringRef(const STRCONST * string);
    mdToken GetModuleToken();
    mdToken GetGlobalFieldDef(METHSYM * sym, unsigned int count, TYPESYM * type, unsigned int size = 0);
    mdToken GetGlobalFieldDef(METHSYM * sym, unsigned int count, unsigned int size, BYTE ** pBuffer);

    void DefineTokenLoc(int offset);
    void Copy(EMITTER &emit);

#ifdef DEBUG
    // IMapToken implementation, 
    STDMETHOD(QueryInterface)(REFIID riid,void __RPC_FAR *__RPC_FAR *ppvObject);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();
    STDMETHOD(Map)(mdToken tkImp, mdToken tkEmit);
#endif //DEBUG

protected:

    void CheckHR(HRESULT hr);
    void CheckHR(int errid, HRESULT hr);
    void CheckHRDbg(HRESULT hr);
    void MetadataFailure(HRESULT hr);
    void DebugFailure(HRESULT hr);
    void MetadataFailure(int errid, HRESULT hr);
    DWORD FlagsFromAccess(ACCESS access);
    PCOR_SIGNATURE BeginSignature();
    PCOR_SIGNATURE EmitSignatureByte(PCOR_SIGNATURE curSig, BYTE b);
    PCOR_SIGNATURE EmitSignatureUInt(PCOR_SIGNATURE curSig, ULONG b);
    PCOR_SIGNATURE EmitSignatureToken(PCOR_SIGNATURE curSig, mdToken token);
    PCOR_SIGNATURE EmitSignatureType(PCOR_SIGNATURE sig, PTYPESYM type);
    PCOR_SIGNATURE GrowSignature(PCOR_SIGNATURE curSig);
    PCOR_SIGNATURE EnsureSignatureSize(ULONG cb);
    PCOR_SIGNATURE EndSignature(PCOR_SIGNATURE curSig, int * cbSig);
    PCOR_SIGNATURE SignatureOfMembVar(PMEMBVARSYM sym, int * cbSig);
    PCOR_SIGNATURE SignatureOfMethodOrProp(PMETHPROPSYM sym, int * cbSig);
    BYTE GetConstValue(PMEMBVARSYM sym, LPVOID tempBuff, LPVOID * ppValue, size_t * pcb);
    bool VariantFromConstVal(TYPESYM * type, CONSTVAL * cv, VARIANT * v);
    mdToken GetScopeForTypeRef(SYM *sym);
    void EmitDebugNamespace(NSSYM * ns);

    void RecordEmitToken(mdToken * tokref);
    void EraseEmitTokens();
    void RecordGlobalToken(mdToken token, INFILESYM *infile);

    void HandleAttributeError(HRESULT hr, BASENODE *parseTree, INFILESYM *infile, METHSYM *method);
    void SaveSecurityAttribute(mdToken token, mdToken ctorToken, METHSYM * method, BYTE * buffer, unsigned bufferSize);
    void FreeSavedSecurityAttributes();

    // For accumulating security attributes
    SECATTRSAVE * listSecAttrSave;
    unsigned cSecAttrSave;
    mdToken tokenSecAttrSave;

    // cache a local copy of these
    IMetaDataEmit* metaemit;
    IMetaDataAssemblyEmit *metaassememit;
    ISymUnmanagedWriter * debugemit;

    // Scratch area for signature creation.
#ifdef DEBUG
    bool           sigBufferInUse;
#endif
    PCOR_SIGNATURE sigBuffer;
    PCOR_SIGNATURE sigEnd;    // End of allocated area.

    // Heap for storing token addresses.
    struct TOKREFS {
        struct TOKREFS * next;
        mdToken * tokenAddrs[CTOKREF];
    };
    NRHEAP tokrefHeap;                  // Heap to allocate from.
    TOKREFS * tokrefList;               // Head of the tokref list.
    int iTokrefCur;                     // Current index within tokrefList.

    NRMARK mark;
};
