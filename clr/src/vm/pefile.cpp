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
// File.CPP
// 

// PEFILE reads in the PE file format using LoadLibrary
// ===========================================================================

#include "common.h"
#include <shlwapi.h>
#include "timeline.h"
#include "eeconfig.h"
#include "pefile.h"
#include "peverifier.h"
#include "security.h"

// ===========================================================================
// PEFile
// ===========================================================================

PEFile::PEFile()
{
    m_wszSourceFile[0] = 0;
    m_hModule = NULL;
    m_hCorModule = NULL;
    m_base = NULL;
    m_pNT = NULL;
    m_pCOR = NULL;
    m_pMDInternalImport = NULL;
    m_pLoadersFileName = NULL;
    m_flags = 0;
    m_dwUnmappedFileLen = (DWORD) -1;
    m_fShouldFreeModule = FALSE;
    m_fHashesVerified = FALSE;
}

PEFile::PEFile(PEFile *pFile)
{
    wcscpy(m_wszSourceFile, pFile->m_wszSourceFile);
    m_hModule = pFile->m_hModule;
    m_hCorModule = pFile->m_hCorModule;
    m_base = pFile->m_base;
    m_pNT = pFile->m_pNT;
    m_pCOR = pFile->m_pCOR;

    m_pMDInternalImport = NULL;
    m_pLoadersFileName = NULL;
    m_flags = pFile->m_flags;

    m_dwUnmappedFileLen = pFile->m_dwUnmappedFileLen;
    m_fShouldFreeModule = FALSE;
}

PEFile::~PEFile()
{
    if (m_pMDInternalImport != NULL)
    {
        m_pMDInternalImport->Release();
    }


    if (m_hCorModule)
        CorMap::ReleaseHandle(m_hCorModule);
    else if(m_fShouldFreeModule)
    {
        _ASSERTE(m_hModule);
        // Unload the dll so that refcounting of EE will be done correctly
        // But, don't do this during process detach (this can be indirectly
        // called during process detach, potentially causing an AV).
        if (!g_fProcessDetach)
            FreeLibrary(m_hModule);
    }

}

HRESULT PEFile::ReadHeaders()
{
    IMAGE_DOS_HEADER* pDos;
    BOOL fData = FALSE;
    if(m_hCorModule) {
        DWORD types = CorMap::ImageType(m_hCorModule);
        if(types == CorLoadOSMap || types == CorLoadDataMap || types == CorLoadOSImage) 
            fData = TRUE;
    }
    return CorMap::ReadHeaders(m_base, &pDos, &m_pNT, &m_pCOR, fData, m_dwUnmappedFileLen);
}

BYTE *PEFile::RVAToPointer(DWORD rva)
{
    if (rva == 0)
        return NULL;

    if(m_hCorModule) {
        DWORD types = CorMap::ImageType(m_hCorModule);
        if(types == CorLoadOSMap || types == CorLoadDataMap || types == CorLoadOSImage) 
        {
            rva = Cor_RtlImageRvaToOffset(m_pNT, rva, GetUnmappedFileLength());
        }
    }

    return m_base + rva;
}

HRESULT PEFile::Create(HMODULE hMod, PEFile **ppFile, BOOL fShouldFree)
{
    HRESULT hr;

    PEFile *pFile = new PEFile();
    if (pFile == NULL)
        return E_OUTOFMEMORY;

    pFile->m_hModule = hMod;
    pFile->m_fShouldFreeModule = fShouldFree;

    pFile->m_base = (BYTE*) hMod;

    hr = pFile->ReadHeaders();
    if (FAILED(hr))
    {
        delete pFile;
        return hr;
    }

    *ppFile = pFile;
    return pFile->GetFileNameFromImage();
}

HRESULT PEFile::Create(HCORMODULE hMod, PEFile **ppFile, BOOL fResource/*=FALSE*/)
{
    HRESULT hr;

    PEFile *pFile = new PEFile();
    if (pFile == NULL)
        return E_OUTOFMEMORY;
    hr = Setup(pFile, hMod, fResource);
    if (FAILED(hr))
        delete pFile;
    else
        *ppFile = pFile;
    return hr;
}

HRESULT PEFile::Setup(PEFile* pFile, HCORMODULE hMod, BOOL fResource)
{
    HRESULT hr;

    // Release any pointers to the map data and the reload as a proper image
    if (pFile->m_pMDInternalImport != NULL) {
        pFile->m_pMDInternalImport->Release();
        pFile->m_pMDInternalImport = NULL;
    }

    
    pFile->m_hCorModule = hMod;
    IfFailRet(CorMap::BaseAddress(hMod, (HMODULE*) &(pFile->m_base)));
	if(pFile->m_base)
	{
		pFile->m_dwUnmappedFileLen = (DWORD)CorMap::GetRawLength(hMod);
		if (!fResource)
			IfFailRet(hr = pFile->ReadHeaders());

		return pFile->GetFileNameFromImage();
	}
	else return E_FAIL;
}

HRESULT PEFile::Create(PBYTE pUnmappedPE, DWORD dwUnmappedPE, LPCWSTR imageNameIn,
                       LPCWSTR pLoadersFileName, 
                       OBJECTREF* pExtraEvidence,
                       PEFile **ppFile, 
                       BOOL fResource)
{
    HCORMODULE hMod = NULL;

    HRESULT hr = CorMap::OpenRawImage(pUnmappedPE, dwUnmappedPE, imageNameIn, &hMod, fResource);
    if (SUCCEEDED(hr) && hMod == NULL)
        hr = E_FAIL;

    if (SUCCEEDED(hr))
    {
        if (fResource)
            hr = Create(hMod, ppFile, fResource);
        else
            hr = VerifyModule(hMod, 
                              NULL, 
                              NULL, 
                              NULL,  // Code base
                              pExtraEvidence, 
                              imageNameIn, NULL, ppFile);
    }

    if (SUCCEEDED(hr)) {
        (*ppFile)->m_pLoadersFileName = pLoadersFileName;
        (*ppFile)->m_dwUnmappedFileLen = dwUnmappedPE;
    }

    return hr;
}


HRESULT PEFile::Create(LPCWSTR moduleName, 
                       Assembly* pParent,
                       mdFile kFile,                 // File token in the parent assembly associated with the file
                       BOOL fIgnoreVerification, 
                       IAssembly* pFusionAssembly,
                       LPCWSTR pCodeBase,
                       OBJECTREF* pExtraEvidence,
                       PEFile **ppFile)
{    
    HRESULT hr;
    _ASSERTE(moduleName);
    LOG((LF_CLASSLOADER, LL_INFO10, "PEFile::Create: Load module: \"%ws\" Ignore Verification = %d.\n", moduleName, fIgnoreVerification));
    
    TIMELINE_START(LOADER, ("PEFile::Create %S", moduleName));

    
    if((fIgnoreVerification == FALSE) || pParent) {
        // ----------------------------------------
        // Verify the module to see if we are allowed to load it. If it has no
        // unexplainable reloc's then it is veriable. If it is a simple 
        // image then we have already loaded it so just return that one.
        // If it is a complex one and security says we can load it then
        // we must release the original interface to it and then reload it.
        // VerifyModule will return S_OK if we do not need to reload it.
        Thread* pThread = GetThread();
        
        IAssembly* pOldFusionAssembly = pThread->GetFusionAssembly();
        Assembly* pAssembly = pThread->GetAssembly();
        mdFile kOldFile = pThread->GetAssemblyModule();

        pThread->SetFusionAssembly(pFusionAssembly);
        pThread->SetAssembly(pParent);
        pThread->SetAssemblyModule(kFile);

        HCORMODULE hModule;
        hr = CorMap::OpenFile(moduleName, CorLoadOSMap, &hModule);

        if (SUCCEEDED(hr)) {
            if(hr == S_FALSE) {
                PEFile *pFile;
                hr = Create(hModule, &pFile);
                if (SUCCEEDED(hr) && pFusionAssembly)
                    hr = ReleaseFusionMetadataImport(pFusionAssembly);
                if (SUCCEEDED(hr))
                    *ppFile = pFile;
            }
            else
                hr = VerifyModule(hModule, 
                                  pParent, 
                                  pFusionAssembly, 
                                  pCodeBase,
                                  pExtraEvidence, 
                                  moduleName,  
                                  NULL, ppFile);
        }

        pThread->SetFusionAssembly(pOldFusionAssembly);
        pThread->SetAssembly(pAssembly);
        pThread->SetAssemblyModule(kOldFile);

        if(pOldFusionAssembly)
            pOldFusionAssembly->Release();

    }
    else {
        
        HCORMODULE hModule;
        hr = CorMap::OpenFile(moduleName, CorLoadOSImage, &hModule);
        if(hr == S_FALSE) // if S_FALSE then we have already loaded it correctly
            hr = Create(hModule, ppFile, FALSE);
        else if(hr == S_OK) // Convert to an image
            hr = CreateImageFile(hModule, pFusionAssembly, ppFile);
        // return an error
        
    }
    TIMELINE_END(LOADER, ("PEFile::Create %S", moduleName));
    return hr;
}

HRESULT PEFile::CreateResource(LPCWSTR moduleName,
                               PEFile **pFile)
{
    HRESULT hr = S_OK;
    HCORMODULE hModule;
    IfFailRet(CorMap::OpenFile(moduleName, CorLoadOSMap,  &hModule));
    return Create(hModule, pFile, TRUE);
}


HRESULT PEFile::Clone(PEFile *pFile, PEFile **ppFile)
{
    PEFile *result;

    result = new PEFile(pFile);
    if (result == NULL)
        return E_OUTOFMEMORY;

    //
    // Add a reference to the file
    //

    if (result->m_hModule != NULL)
    {
#ifdef _DEBUG
        HMODULE hMod =
#endif
        WszLoadLibrary(result->m_wszSourceFile);

        // Note that this assert may fail on win 9x for .exes.  This is a 
        // design problem which is being corrected - soon we will not allow
        // binding to .exes
            _ASSERTE(hMod == result->m_hModule);
            result->m_fShouldFreeModule = TRUE;
        }
    else if (result->m_hCorModule != NULL)
    {
#ifdef _DEBUG
        HRESULT hr =
#endif
        CorMap::AddRefHandle(result->m_hCorModule);
        _ASSERTE(hr == S_OK);
    }

    *ppFile = result;
    return S_OK;
}

IMDInternalImport *PEFile::GetMDImport(HRESULT *phr)
{
    HRESULT hr = S_OK;

    if (m_pMDInternalImport == NULL)
    {
        _ASSERTE(m_pCOR != NULL);

        IMAGE_DATA_DIRECTORY *pMeta = &m_pCOR->MetaData;

        DWORD offset;
        // Find the meta-data. If it is a non-mapped image then use the base offset
        // instead of the virtual address
        if(m_hCorModule) {
            DWORD flags = CorMap::ImageType(m_hCorModule);
            if(flags == CorLoadImageMap || flags == CorReLoadOSMap) 
            {
                offset = VAL32(pMeta->VirtualAddress);
            }
            else
            {
                offset = Cor_RtlImageRvaToOffset(m_pNT,
                                                 VAL32(pMeta->VirtualAddress),
                                                 GetUnmappedFileLength());
            }
        }
        else 
            offset = VAL32(pMeta->VirtualAddress);

        // Range check the metadata blob
        if (Cor_RtlImageRvaRangeToSection(m_pNT,
                                          VAL32(pMeta->VirtualAddress),
                                          VAL32(pMeta->Size),
                                          GetUnmappedFileLength()))
        {

            hr = GetMetaDataInternalInterface(m_base + offset,
                                              VAL32(pMeta->Size),
                                              ofRead, 
                                              IID_IMDInternalImport,
                                              (void **) &m_pMDInternalImport);
        }
        else
            hr = HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
    }

    if (phr)
        *phr = hr;

    return m_pMDInternalImport;
}

HRESULT PEFile::VerifyFlags(DWORD flags, BOOL fZap)
{

    DWORD validBits = COMIMAGE_FLAGS_ILONLY | COMIMAGE_FLAGS_32BITREQUIRED | COMIMAGE_FLAGS_TRACKDEBUGDATA | COMIMAGE_FLAGS_STRONGNAMESIGNED;
    if (fZap)
        validBits |= COMIMAGE_FLAGS_IL_LIBRARY;

    DWORD mask = ~validBits;

    if (!(flags & mask))
        return S_OK;

    return HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
}

IMAGE_DATA_DIRECTORY *PEFile::GetSecurityHeader()
{
    if (m_pNT == NULL)
        return NULL;
    else
        return &m_pNT->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY];
}

HRESULT PEFile::SetFileName(LPCWSTR codeBase)
{
    if(codeBase == NULL)
        return E_INVALIDARG;

    DWORD lgth = (DWORD) wcslen(codeBase) + 1;
    if(lgth > MAX_PATH)
        return HRESULT_FROM_WIN32(ERROR_BAD_PATHNAME);

    wcscpy(m_wszSourceFile, codeBase);
    return S_OK;
}


LPCWSTR PEFile::GetFileName()
{
    return m_wszSourceFile;
}

LPCWSTR PEFile::GetLeafFileName()
{
    WCHAR *pStart = m_wszSourceFile;
    WCHAR *pEnd = pStart + wcslen(m_wszSourceFile);
    WCHAR *p = pEnd;
    
    while (p > pStart)
    {
        if (*--p == '\\')
        {
            p++;
            break;
        }
    }

    return p;
}

HRESULT PEFile::GetFileNameFromImage()
{
    HRESULT hr = S_OK;

    DWORD dwSourceFile = 0;

    if (m_hCorModule)
    {
        CorMap::GetFileName(m_hCorModule, m_wszSourceFile, MAX_PATH, &dwSourceFile);
    }
    else if (m_hModule)
    {
        dwSourceFile = WszGetModuleFileName(m_hModule, m_wszSourceFile, MAX_PATH);
        if (dwSourceFile == 0)
        {
            *m_wszSourceFile = 0;
            hr = HRESULT_FROM_WIN32(GetLastError());
            if (SUCCEEDED(hr)) // GetLastError doesn't always do what we'd like
                hr = E_FAIL;
        }

        if (dwSourceFile == MAX_PATH)
        {
            // Since dwSourceFile doesn't include the null terminator, this condition
            // implies that the file name was truncated.  We cannot 
            // currently tolerate this condition.
            // @nice: add logic to handle larger paths
            return HRESULT_FROM_WIN32(ERROR_CANNOT_MAKE);
        }
        else
        dwSourceFile++; // add in the null terminator
    }

    if (SystemDomain::System()->IsSystemFile(m_wszSourceFile))
        m_flags |= PEFILE_SYSTEM;

    _ASSERTE(dwSourceFile <= MAX_PATH);

    return hr;
}

HRESULT PEFile::GetFileName(LPSTR psBuffer, DWORD dwBuffer, DWORD* pLength)
{
    if (m_hCorModule)
    {
        CorMap::GetFileName(m_hCorModule, psBuffer, dwBuffer, pLength);
    }
    else if (m_hModule)
    {
        DWORD length = GetModuleFileNameA(m_hModule, psBuffer, dwBuffer);
        if (length == 0)
        {
            HRESULT hr = HRESULT_FROM_WIN32(GetLastError());
            if (SUCCEEDED(hr)) // GetLastError doesn't always do what we'd like
                hr = E_FAIL;
        }
        *pLength = length;
    }
    else
    {
        *pLength = 0;
    }

    return S_OK;
}

/*** For reference, from ntimage.h
    typedef struct _IMAGE_TLS_DIRECTORY {
        ULONG   StartAddressOfRawData;
        ULONG   EndAddressOfRawData;
        PULONG  AddressOfIndex;
        PIMAGE_TLS_CALLBACK *AddressOfCallBacks;
        ULONG   SizeOfZeroFill;
        ULONG   Characteristics;
    } IMAGE_TLS_DIRECTORY;
***/

IMAGE_TLS_DIRECTORY* PEFile::GetTLSDirectory() 
{
    return NULL;
}

BOOL PEFile::IsTLSAddress(void* address)  
{
    IMAGE_TLS_DIRECTORY* tlsDir = GetTLSDirectory();
    if (tlsDir == 0)
        return FALSE;

    size_t asInt = (size_t) address;

    return (tlsDir->StartAddressOfRawData <= asInt 
            && asInt < tlsDir->EndAddressOfRawData);
}

LPCWSTR PEFile::GetLoadersFileName()
{
    return m_pLoadersFileName;
}

HRESULT PEFile::FindCodeBase(WCHAR* pCodeBase, 
                             DWORD* pdwCodeBase)
{
    LPWSTR pFileName = (LPWSTR) GetFileName();
    CQuickWSTR buffer;
    
    // Cope with the case where we've been loaded from a byte array and
    // don't have a code base of our own. In this case we should have cached
    // the file name of the assembly that loaded us.
    if (pFileName[0] == L'\0') {
        pFileName = (LPWSTR) GetLoadersFileName();
        
        if (!pFileName) {
            HRESULT hr;
            if (FAILED(hr = buffer.ReSize(MAX_PATH)))
                return hr;

            pFileName = buffer.Ptr();

            DWORD dwFileName = WszGetModuleFileName(NULL, pFileName, MAX_PATH);
            if (dwFileName == MAX_PATH)
            {
                // Since dwSourceFile doesn't include the null terminator, this condition
                // implies that the file name was truncated.  We cannot 
                // currently tolerate this condition. (We can't reallocate the buffer
                // since we don't know how big to make it.)
                return HRESULT_FROM_WIN32(ERROR_CANNOT_MAKE);
            }

            LOG((LF_CLASSLOADER, LL_INFO10, "Found codebase from OSHandle: \"%ws\".\n", pFileName));
        }
    }
    
    return FindCodeBase(pFileName, pCodeBase, pdwCodeBase);
}

HRESULT PEFile::FindCodeBase(LPCWSTR pFileName, 
                             WCHAR* pCodeBase, 
                             DWORD* pdwCodeBase)
{
    if(pFileName == NULL || *pFileName == 0) return E_FAIL;

    HRESULT hr;

    if (*pdwCodeBase == 0) {
        *pdwCodeBase = 1;
        wchar_t wszUrl[1];
        UrlCanonicalize(pFileName, wszUrl, pdwCodeBase, URL_UNESCAPE);
        *pdwCodeBase += 1;
        return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);           
    }

    hr = UrlCanonicalize(pFileName, pCodeBase, pdwCodeBase, URL_UNESCAPE);
    
    LOG((LF_CLASSLOADER, LL_INFO10, "Created codebase: \"%ws\".\n", pCodeBase));
    return hr;
}



// We need to verify a module to see if it verifiable. 
// Returns:
//    1) Module has been previously loaded and verified
//         1a) loaded via LoadLibrary
//         1b) loaded using CorMap
//    2) The module is veriable
//    3) The module is not-verifable but allowed
//    4) The module is not-verifable and not allowed
HRESULT PEFile::VerifyModule(HCORMODULE hModule,
                             Assembly* pParent,      
                             IAssembly *pFusionAssembly,
                             LPCWSTR pCodeBase,
                             OBJECTREF* pExtraEvidence,
                             LPCWSTR moduleName,
                             HCORMODULE *phModule,
                             PEFile** ppFile)
{
    HRESULT hr = S_OK;
    PEFile* pImage = NULL;

    DWORD dwFileLen;
    dwFileLen = (DWORD)CorMap::GetRawLength(hModule);
    if(dwFileLen == (DWORD) -1)
        return HRESULT_FROM_WIN32(ERROR_INVALID_HANDLE);
        
    HMODULE pHandle;
    IfFailRet(CorMap::BaseAddress(hModule, &pHandle));
    PEVerifier pe((PBYTE)pHandle, dwFileLen);
        
        if (!pe.Check()) {
            // It is not verifiable so we need to map it with out calling any entry
            // points and then ask the security system whether we are allowed to 
            // load this Image.
            
        // Release the metadata pointer if it exists. This has been passed in to
        // maintain the life time of the image if it first loaded by fusion. We
        // now have our own ref count with hModule.

        LOG((LF_CLASSLOADER, LL_INFO10, "Module is not verifiable: \"%ws\".\n", moduleName));
            
        // Remap the image, if it has been loaded by fusion for meta-data
        // then the image will be in data format.

        // Load as special unmapped version of a PEFile
        hr = Create((HCORMODULE)hModule, &pImage);
        hModule = NULL; // So we won't release it if error
        IfFailGo(hr);

        if (Security::IsSecurityOn())
        {
            if(pParent) {
                AssemblySecurityDescriptor *pAssemblySec = pParent->GetSecurityDescriptor();
                if(!pAssemblySec || !pAssemblySec->CanSkipVerification()) {
                    LOG((LF_CLASSLOADER, LL_INFO10, "Module fails to load because assembly module did not get granted permission: \"%ws\".\n", moduleName));
                    IfFailGo(SECURITY_E_UNVERIFIABLE);
                }
            }
            else {
                PEFile* pSecurityImage = NULL;

                CorMap::AddRefHandle(pImage->GetCORModule());
                hr = Create(pImage->GetCORModule(), &pSecurityImage);
                IfFailGo(hr);
            
                if(pCodeBase)
                    pSecurityImage->SetFileName(pCodeBase);

                if(Security::CanLoadUnverifiableAssembly(pSecurityImage, pExtraEvidence) == FALSE) {
                    LOG((LF_CLASSLOADER, LL_INFO10, "Module fails to load because assembly did not get granted permission: \"%ws\".\n", moduleName));
                    delete pSecurityImage;
                    IfFailGo(SECURITY_E_UNVERIFIABLE);
                }
                delete pSecurityImage;
            }
        }

        // Release the fusion handle
        if(pFusionAssembly) 
            IfFailGo(ReleaseFusionMetadataImport(pFusionAssembly));

            // Remap the image using the OS loader
        HCORMODULE pResult;
        IfFailGo(CorMap::MemoryMapImage(pImage->m_hCorModule, &pResult));
        if(pImage->m_hCorModule != pResult) {
            pImage->m_hCorModule = pResult;
        }

        // The image has changed so we need to set up the PEFile
        // with the correct addresses.
        IfFailGo(Setup(pImage, pImage->m_hCorModule, FALSE));
        *ppFile = pImage;

        // It is not verifable but it is allowed to be loaded.
        return S_OK;
    }            
    else {
        return CreateImageFile(hModule, pFusionAssembly, ppFile);
    }
    

 ErrExit:
#ifdef _DEBUG
    LOG((LF_CLASSLOADER, LL_INFO10, "Failed to load module: \"%ws\". Error %x\n", moduleName, hr));
#endif //_DEBUG
    
    // On error try and release the handle;
    if(pImage)
        delete pImage;
    else if(hModule)
        CorMap::ReleaseHandle(hModule); // Ignore error

    return hr;
}

HRESULT PEFile::CreateImageFile(HCORMODULE hModule, IAssembly* pFusionAssembly, PEFile **ppFile)
{
    HRESULT hr = S_OK;

    // Release the fusion handle
    if(pFusionAssembly) 
        IfFailRet(ReleaseFusionMetadataImport(pFusionAssembly));

    
    hr = Create(hModule, ppFile, FALSE);
    return hr;
}

const IID IID_IMetaDataAssemblyImportControl = 
    { 0xcc8529d9, 0xf336,0x471b, { 0xb6, 0x0a, 0xc7, 0xc8, 0xee, 0x9b, 0x84, 0x92 } };

HRESULT PEFile::ReleaseFusionMetadataImport(IAssembly* pAsm)
{
    HRESULT hr = S_OK;
    IServiceProvider *pSP;
    IAssemblyManifestImport *pAsmImport;
    IMetaDataAssemblyImportControl *pMDAIControl;

    hr = pAsm->QueryInterface(IID_IServiceProvider, (void **)&pSP);
    if (hr == S_OK && pSP) {
        hr = pSP->QueryService(IID_IAssemblyManifestImport, 
                               IID_IAssemblyManifestImport, (void**)&pAsmImport);
        if (hr == S_OK && pAsmImport) {
            hr = pAsmImport->QueryInterface(IID_IMetaDataAssemblyImportControl, 
                                            (void **)&pMDAIControl);
        
            if (hr == S_OK && pMDAIControl) {
                IUnknown* pImport = NULL;
                // Temporary solution until fusion makes this generic
                CorMap::EnterSpinLock();
                // This may return an error if we have already
                // released the import.
                pMDAIControl->ReleaseMetaDataAssemblyImport((IUnknown**)&pImport);
                CorMap::LeaveSpinLock();
                if(pImport != NULL)
                    pImport->Release();
                pMDAIControl->Release();
            }
            pAsmImport->Release();
        }
        pSP->Release();
    }
    return hr;
}


//================================================================
