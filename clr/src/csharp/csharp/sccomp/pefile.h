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
// File: pefile.h
//
// Defines the structures used to create executable files.
// ===========================================================================

#include <iceefilegen.h>

class ASSEMBLYEMITTER; // So these guys can get at these fields
class EMITTER;

class CPEFile {
public:
	CPEFile();
    void Init( COMPILER *comp);
    void Term();

    bool BeginOutputFile(POUTFILESYM pOutFile);
    bool EndOutputFile(bool bWriteFile);
    void SetAttributes(bool fDll);
    void *AllocateRVABlock(ULONG cb, ULONG alignment, ULONG *codeRVA, ULONG *offset);
    void *GetCodeLocationOfOffset(ULONG offset);
    ULONG GetRVAOfOffset(ULONG offset);
    void DefineStringLoc(int offset);
    void WriteCryptoKey();
    bool CalcResource(PRESFILESYM pRes);
    bool AllocResourceBlob();
    bool AddResource(PRESFILESYM pRes);

    static void ReplaceFileExtension(LPWSTR lpszFilename, LPCWSTR lpszExt); // Note that lpszExt must contain the dot
    static LPCWSTR GetModuleName(POUTFILESYM pOutFile, COMPILER * compiler); // Gets the module name, do not free!


    // accessors
    IMetaDataEmit          *GetEmit()               { return metaemit; }
    IMetaDataAssemblyEmit  *GetAssemblyEmitter()    { return assememit; }
    POUTFILESYM             GetOutFile()            { return outfile; }
    ISymUnmanagedWriter *   GetDebugEmit()          { return debugemit; }

    // Image helpers
    // These are just wrappers so the compiler doesn't have to deal with 32/64 bit headers
    static PVOID Cor_RtlImageRvaToVa(PVOID Base, ULONG Rva);
    static PIMAGE_SECTION_HEADER Cor_RtlImageRvaToSection(PVOID Base, ULONG Rva);

    static void GetPDBFileName(OUTFILESYM * pOutFile, LPWSTR filename);
    static HRESULT CreateSymbolWriter(ISymUnmanagedWriter ** pSymwriter);
    static HRESULT CreateSymbolReader(ISymUnmanagedReader ** pSymreader);

private:

    IMetaDataEmit           *metaemit;
    ISymUnmanagedWriter *   debugemit;
    IMetaDataAssemblyEmit   *assememit;
    OUTFILESYM              *outfile;
    
    CComPtr<IStream>        symbolStream;

    HCEEFILE                ceeFile;
    HCEESECTION             ilSection, rdataSection;

    DWORD                   m_dwBaseResRva;
    DWORD                   m_dwResSize;
    void                    *m_pResBuffer;
    DWORD                   m_dwOffset;

    ICeeFileGen             *ceefilegen;

    COMPILER                *compiler;

    // block location in PE of last method emitted, for use by DefineTokenLoc.
    ULONG lastMethodLoc;     


    void CheckHR(HRESULT hr);
    void CheckHR(int errid, HRESULT hr);
    void MetadataFailure(HRESULT hr);
    void MetadataFailure(int errid, HRESULT hr);

    static void FindNTHeader(PBYTE pbMapAddress, PIMAGE_NT_HEADERS32 *ppNT32, PIMAGE_NT_HEADERS64 *ppNT64);
    static PVOID Cor_RtlImageRvaToVa(PIMAGE_NT_HEADERS64 NtHeaders, PVOID Base, ULONG Rva);
    static PIMAGE_SECTION_HEADER Cor_RtlImageRvaToSection(PIMAGE_NT_HEADERS64 NtHeaders, PVOID Base, ULONG Rva);
    static PVOID Cor_RtlImageRvaToVa(PIMAGE_NT_HEADERS32 NtHeaders, PVOID Base, ULONG Rva);
    static PIMAGE_SECTION_HEADER Cor_RtlImageRvaToSection(PIMAGE_NT_HEADERS32 NtHeaders, PVOID Base, ULONG Rva);

};
