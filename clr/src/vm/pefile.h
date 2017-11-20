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
// File: PEFILE.H
// 

// PEFILE.H defines the class use to represent the PE file
// ===========================================================================
#ifndef PEFILE_H_
#define PEFILE_H_

#include <windef.h>

#include <fusion.h>
#include <fusionpriv.h>
#include "vars.hpp" // for LPCUTF8
#include "hash.h"
#include "cormap.hpp"
#include <member-offset-info.h>

//
// A PEFile is the runtime's abstraction of an executable image.
// It may or may not have an actual file associated with it.  PEFiles
// exist mostly to be turned into Modules later.
//

enum PEFileFlags {
    PEFILE_SYSTEM = 0x1,
};

class PEFile
{
    friend struct MEMBER_OFFSET_INFO(PEFile);
    friend HRESULT InitializeMiniDumpBlock();

private:
    WCHAR               m_wszSourceFile[MAX_PATH];

    HMODULE             m_hModule;
    HCORMODULE          m_hCorModule;
    BYTE                *m_base;
    IMAGE_NT_HEADERS    *m_pNT;
    IMAGE_COR20_HEADER  *m_pCOR;
    IMDInternalImport   *m_pMDInternalImport;
    LPCWSTR             m_pLoadersFileName;
    DWORD               m_flags;
    DWORD               m_dwUnmappedFileLen; //for resource files, Win9X, and byte[] files
    BOOL                m_fShouldFreeModule;
    BOOL                m_fHashesVerified; // For manifest files, have internal modules been verified by fusion

    PEFile();
    PEFile(PEFile *pFile);

    HRESULT GetFileNameFromImage();

public:

    ~PEFile();

    static HRESULT Create(HMODULE hMod, PEFile **ppFile, BOOL fShouldFree);
    static HRESULT Create(HCORMODULE hMod, PEFile **ppFile, BOOL fResource=FALSE);
    static HRESULT Create(LPCWSTR moduleNameIn,         // Name of the PE image
                          Assembly* pParent,            // If file is a module you need to pass in the Assembly
                          mdFile kFile,                 // File token in the parent assembly associated with the file
                          BOOL fIgnoreVerification,     // Do not check entry points before loading
                          IAssembly* pFusionAssembly,   // Fusion object associated with module
                          LPCWSTR pCodeBase,            // Location where image originated (if different from name)
                          OBJECTREF* pExtraEvidence,    // Evidence that relates to the image (eg. zone, URL)
                          PEFile **ppFile);             // Returns a PEFile
    static HRESULT Create(PBYTE pUnmappedPE, DWORD dwUnmappedPE, 
                          LPCWSTR imageNameIn,
                          LPCWSTR pLoadersFileName, 
                          OBJECTREF* pExtraEvidence,    // Evidence that relates to the image (eg. zone, URL)
                          PEFile **ppFile,              // Returns a PEFile
                          BOOL fResource);
    static HRESULT CreateResource(LPCWSTR moduleNameIn,         // Name of the PE image
                                  PEFile **ppFile);             // Returns a PEFile

    static HRESULT VerifyModule(HCORMODULE hModule,
                                Assembly* pParent,
                                IAssembly* pFusionAssembly, 
                                LPCWSTR pCodeBase,
                                OBJECTREF* pExtraEvidence,
                                LPCWSTR pName,
                                HCORMODULE *phModule,
                                PEFile** ppFile);

    static HRESULT CreateImageFile(HCORMODULE hModule, 
                                   IAssembly* pFusionAssembly, 
                                   PEFile **ppFile);

    static HRESULT Clone(PEFile *pFile, PEFile **ppFile);

    BYTE *GetBase()
    { 
        return m_base; 
    }
    IMAGE_NT_HEADERS *GetNTHeader()
    { 
        return m_pNT; 
    }
    IMAGE_COR20_HEADER *GetCORHeader() 
    { 
        return m_pCOR; 
    }
    BYTE *RVAToPointer(DWORD rva);

    HCORMODULE GetCORModule()
    {
        return m_hCorModule;
    }
    HRESULT ReadHeaders();
    
    void ShouldDelete()
    {
        m_fShouldFreeModule = TRUE;
    }

    BOOL IsSystem() { return (m_flags & PEFILE_SYSTEM) != 0; }

    IMAGE_DATA_DIRECTORY *GetSecurityHeader();

    IMDInternalImport *GetMDImport(HRESULT *hr = NULL);
    IMDInternalImport *GetZapMDImport(HRESULT *hr = NULL);

    HRESULT VerifyFlags(DWORD flag, BOOL fZap);

    BOOL IsTLSAddress(void* address);
    IMAGE_TLS_DIRECTORY* GetTLSDirectory();

    HRESULT SetFileName(LPCWSTR codeBase);
    LPCWSTR GetFileName();
    LPCWSTR GetLeafFileName();

    LPCWSTR GetLoadersFileName();

    //for resource files, Win9X, and byte[] files
    DWORD GetUnmappedFileLength()
    {
        return m_dwUnmappedFileLen;
    }

    HRESULT FindCodeBase(WCHAR* pCodeBase, 
                         DWORD* pdwCodeBase);

    static HRESULT FindCodeBase(LPCWSTR pFileName, 
                                WCHAR* pCodeBase, 
                                DWORD* pdwCodeBase);

    

    HRESULT GetFileName(LPSTR name, DWORD max, DWORD *count);

    HRESULT VerifyDirectory(IMAGE_DATA_DIRECTORY *dir, DWORD dwForbiddenCharacteristics)
    {
        return CorMap::VerifyDirectory(m_pNT, dir, dwForbiddenCharacteristics);
    }

    void SetHashesVerified()
    {
        m_fHashesVerified = TRUE;
    }

    BOOL HashesVerified()
    {
        return m_fHashesVerified;
    }

    static HRESULT ReleaseFusionMetadataImport(IAssembly* pAsm);

private:
    static HRESULT Setup(PEFile* pFile,
                         HCORMODULE hMod,
                         BOOL fResource);
};



#endif
