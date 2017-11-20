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
// File: asmlink.cpp
//
// ===========================================================================

#include "stdafx.h"
#include "asmlink.h"

// {4E9DB6FB-FF03-4398-9FCF-EAA0C55311AB}
extern const GUID LIBID_ALinkLib;

// {F7E02368-A7F4-471f-8C5E-9839ED57CB5E}
// The Assembly Manifest Linker class
extern const GUID CLSID_AssemblyLinker;

// {C8E77F39-3604-4fd4-85CF-38BDEB233AD4}
// The interface
extern const GUID IID_IALink;


CAsmLink::CAsmLink()
{
    m_pAssem = NULL;
    m_pStdLib = NULL;
    m_pImports = new CAssemblyFile(NULL, this);
    m_pModules = new CAssemblyFile(NULL, this);
    m_pDisp = NULL;
    m_bInited = false;
    m_bPreClosed = false;
    m_bManifestEmitted = false;
    m_bDontDoHashes = false;
    m_pError = NULL;
}

CAsmLink::~CAsmLink()
{
    if (m_pAssem != NULL)
        delete m_pAssem;
    if (m_pDisp != NULL) {
        m_pDisp->Release();
        m_pDisp = NULL;
    }
    if (m_pError != NULL) {
        m_pError->Release();
        m_pError = NULL;
    }
    delete m_pImports;
    delete m_pModules;
}

// Simple helper function to report an file name too long error.
HRESULT CAsmLink::FileNameTooLong(LPCWSTR filename)
{
    WCHAR buffer[70];

    // Put the first 30 and last 30 characters of the file name in the message.
    memcpy(buffer, filename, sizeof(WCHAR) * 30);
    buffer[30] = '\0';
    wcscat(buffer, L"...");
    wcscat(buffer, filename + wcslen(filename) - 30);

    return m_pImports->ReportError(ERR_FileNameTooLong, mdTokenNil, NULL, buffer);
}



HRESULT CAsmLink::Init(IMetaDataDispenserEx *pDispenser, IMetaDataError *pError)
{

    VARIANT v;
    HRESULT hr;
    ASSERT(!m_bInited && pDispenser != NULL);
    m_pDisp = pDispenser;
    m_pDisp->AddRef();

    memset(&v, 0, sizeof(v));
    V_VT(&v) = VT_EMPTY;
    if (SUCCEEDED(hr = m_pDisp->GetOption(MetaDataCheckDuplicatesFor, &v))) {
        V_UI4(&v) |= MDDupModuleRef | MDDupAssemblyRef | MDDupAssembly;
        hr = m_pDisp->SetOption(MetaDataCheckDuplicatesFor, &v);
    }

    if (pError != NULL) {
        m_pError = pError;
        m_pError->AddRef();
    } else
        m_pError = NULL;
    m_bInited = true;
    return hr;
}

HRESULT CAsmLink::SetAssemblyFile(LPCWSTR pszFilename, IMetaDataEmit *pEmitter, AssemblyFlags afFlags, mdAssembly *pAssemblyID)
{
    ASSERT(m_bInited && !m_bPreClosed && !m_pAssem);
    ASSERT(pEmitter != NULL && pAssemblyID != NULL);
    HRESULT hr = E_FAIL;
    
    // NULL filename means 'InMemory' assembly, but we need a filename so convert it to the
    // empty string.
    if (pszFilename == NULL) {
        ASSERT(afFlags & afInMemory);
        pszFilename = L"";
    }

    if (FAILED(hr = SetNonAssemblyFlags(afFlags)))
        return hr;

    if (wcslen(pszFilename) > MAX_PATH)
        return FileNameTooLong(pszFilename); // File name too long

    CComPtr<IMetaDataAssemblyEmit> pAEmitter;
    hr = pEmitter->QueryInterface(IID_IMetaDataAssemblyEmit, (void**)&pAEmitter);
    if (SUCCEEDED(hr)) {
        m_pAssem = new CAssemblyFile(pszFilename, afFlags, pAEmitter, pEmitter, m_pError, this);
        *pAssemblyID = TokenFromRid(mdtAssembly, 1);
    }
    return hr;
}

HRESULT CAsmLink::SetNonAssemblyFlags(AssemblyFlags afFlags)
{
    ASSERT(m_bInited && !m_bPreClosed);
    HRESULT hr = S_FALSE;
    
    if (afFlags & afNoRefHash) {
        if (!m_bDontDoHashes) {
            // This just changed, so tell all the previously imported assemblies
            DWORD cnt = m_pImports->CountFiles(); 
            for (DWORD d = 0; d < cnt; d++) {
                CAssemblyFile *pFile = NULL;
                if (FAILED(hr = m_pImports->GetFile(d, (CFile**)&pFile)))
                    return hr;
                pFile->DontDoHash();
            }
        }
        m_bDontDoHashes = true;
    }

    return hr;
}

HRESULT CAsmLink::AddFile(mdAssembly AssemblyID, LPCWSTR pszFilename, DWORD dwFlags,
                   IMetaDataEmit *pEmitter, mdFile * pFileToken)
{
    ASSERT(m_bInited && !m_bPreClosed);
    ASSERT(AssemblyID == TokenFromRid(mdtAssembly, 1) || AssemblyID == AssemblyIsUBM);

    // NULL filename means 'InMemory' module, but we need a filename so convert it to the
    // empty string.
    if (pszFilename == NULL)
        pszFilename = L"";


    if (AssemblyID == AssemblyIsUBM) {
        if (m_pAssem == NULL)
            m_pAssem = new CAssemblyFile(m_pError, this);
    }

    if (wcslen(pszFilename) > MAX_PATH)
        return FileNameTooLong(pszFilename); // File name too long

    HRESULT hr = S_OK;
    CFile *file = new CFile(pszFilename, pEmitter, m_pError, this);
    if (FAILED(hr = m_pAssem->AddFile(file, dwFlags, pFileToken)))
        delete file;
    return hr;
}

HRESULT CAsmLink::AddImport(mdAssembly AssemblyID, mdToken ImportToken, DWORD dwFlags, mdFile * pFileToken)
{
    // If we have already emitted the manifest, and then we import
    // a file with a CA that maps to an assembly option, we're in trouble!
    ASSERT(m_bInited && !m_bPreClosed && !m_bManifestEmitted);

    HRESULT hr;
    CFile *file = NULL;
    if (TypeFromToken(ImportToken) == mdtModule) {
        ASSERT(RidFromToken(ImportToken) < m_pModules->CountFiles());
        hr = m_pModules->GetFile( ImportToken, &file);
    } else {
        ASSERT(TypeFromToken(ImportToken) == mdtAssemblyRef && RidFromToken(ImportToken) < m_pImports->CountFiles());
        hr = m_pImports->GetFile( ImportToken, &file);
    }
    if (FAILED(hr))
        return hr;
    ASSERT(file != NULL);
    if (FAILED(hr = m_pAssem->AddFile(file, dwFlags, pFileToken)))
        return hr;
    else if (AssemblyID == AssemblyIsUBM) {
        if (pFileToken)
            *pFileToken = ImportToken;
        return S_FALSE;
    }
    ASSERT(AssemblyID == TokenFromRid(mdtAssembly, 1));
    if (FAILED(hr = m_pModules->RemoveFile( ImportToken)))
        return hr;
    if (FAILED(hr = file->ImportFile( NULL, m_pAssem)))
        return hr;
    return file->ImportResources(m_pAssem);
}

HRESULT CAsmLink::GetAssemblyRefHash(mdToken FileToken, const void** ppvHash, DWORD* pcbHash)
{
    if (TypeFromToken(FileToken) != mdtAssemblyRef) {
        VSFAIL( "You can only get AssemblyRef hashes for assemblies!");
        return E_INVALIDARG;
    }

    HRESULT hr;
    CAssemblyFile *file = NULL;
    if (FAILED(hr = m_pImports->GetFile( FileToken, (CFile**)&file)))
        return hr;

    return file->GetHash(ppvHash, pcbHash);
}

HRESULT CAsmLink::ImportFile(LPCWSTR pszFilename, LPCWSTR pszTargetName, BOOL fSmartImport,
                      mdToken *pFileToken, IMetaDataAssemblyImport **ppAssemblyScope, DWORD *pdwCountOfScopes)
{
    return ImportFile2(pszFilename, pszTargetName, NULL, fSmartImport, pFileToken, ppAssemblyScope, pdwCountOfScopes);
}


HRESULT CAsmLink::ImportFile2(LPCWSTR pszFilename, LPCWSTR pszTargetName, IMetaDataAssemblyImport *pAssemblyScopeIn, 
                      BOOL fSmartImport, mdToken *pFileToken, IMetaDataAssemblyImport **ppAssemblyScope, 
                      DWORD *pdwCountOfScopes)
{
    ASSERT(m_bInited);
    LPWSTR newTarget = NULL;
    bool bNewTarget = false;

    IMetaDataAssemblyImport *pAImport = NULL;
    IMetaDataImport *pImport = NULL;
    mdAssembly tkAssembly;
    if (pszFilename && wcslen(pszFilename) > MAX_PATH)
        return FileNameTooLong(pszFilename);
    if (pszTargetName && wcslen(pszTargetName) > MAX_PATH)
        return FileNameTooLong(pszTargetName); // File name too long

    if (pszTargetName == NULL)
        newTarget = VSAllocStr(pszFilename);
    else
        newTarget = VSAllocStr(pszTargetName);
    
    if (_wcsicmp(newTarget, pszFilename) == 0) {
    } else {
        bNewTarget = true;
    }

    if ((pAImport = pAssemblyScopeIn) != NULL)
    {
        pAImport->AddRef();
    }
    else
    {
        HRESULT hr = m_pDisp->OpenScope(pszFilename, ofRead | ofNoTypeLib, IID_IMetaDataAssemblyImport, (IUnknown**)&pAImport);
        if (FAILED(hr)) {
            *pFileToken = 0;
            if (ppAssemblyScope) *ppAssemblyScope = NULL;
            if (pdwCountOfScopes) *pdwCountOfScopes = 0;
            VSFree(newTarget);
            return hr;
        }
    }

    HRESULT hr;
    VERIFY(SUCCEEDED(hr = pAImport->QueryInterface( IID_IMetaDataImport, (void**)&pImport)));
    hr = pAImport->GetAssemblyFromScope( &tkAssembly);

    if (FAILED(hr)) {
        hr = S_FALSE;
        // This is NOT an assembly
        if (ppAssemblyScope) *ppAssemblyScope = NULL;
        if (pdwCountOfScopes) *pdwCountOfScopes = 1;
        *pFileToken = mdTokenNil;
        CFile* file = new CFile(newTarget, pImport, m_pError, this);
        if (bNewTarget) {
            if (SUCCEEDED(hr = file->SetSource(pszFilename)))
                hr = file->UpdateModuleName();
        }
        pAImport->Release();
        pImport->Release();
        if (SUCCEEDED(hr) && SUCCEEDED(hr = m_pModules->AddFile( file, 0, pFileToken))) {
            if (fSmartImport)
                hr = file->ImportFile( NULL, NULL);
            *pFileToken = TokenFromRid(RidFromToken(*pFileToken), mdtModule);
        }
        if (ppAssemblyScope) *ppAssemblyScope = NULL;
        if (pdwCountOfScopes) *pdwCountOfScopes = 1;
        VSFree(newTarget);
        return hr == S_OK ? S_FALSE : hr;
    } else {
        // It is an Assembly
        CAssemblyFile* assembly = new CAssemblyFile( pszFilename, pAImport, pImport, m_pError, this);
        if (ppAssemblyScope)
            *ppAssemblyScope = pAImport;
        else
            pAImport->Release();
        pImport->Release();
        if (m_bDontDoHashes)
            assembly->DontDoHash();
        hr = assembly->ImportAssembly(pdwCountOfScopes, fSmartImport, m_pDisp);

        if (SUCCEEDED(hr) && bNewTarget)
            hr = assembly->ReportError( ERR_CantRenameAssembly, mdTokenNil, NULL, pszFilename);

        if (FAILED(hr)) {
            delete assembly;
            VSFree(newTarget);
            return hr;
        }

        VSFree(newTarget);
        if (FAILED(hr = m_pImports->AddFile( assembly, 0, pFileToken)))
            delete assembly;
        return SUCCEEDED(hr) ? S_OK : hr;
    }
}

HRESULT CAsmLink::GetScope(mdAssembly AssemblyID, mdToken FileToken, DWORD dwScope, IMetaDataImport** ppImportScope)
{
    ASSERT(m_bInited && !m_bPreClosed);
    ASSERT(AssemblyID == TokenFromRid(mdtAssembly, 1) || AssemblyID == AssemblyIsUBM);
    ASSERT((TypeFromToken(FileToken) == mdtFile && RidFromToken(FileToken) < m_pAssem->CountFiles()) || 
        (TypeFromToken(FileToken) == mdtAssemblyRef && RidFromToken(FileToken) < m_pImports->CountFiles()) || 
        (TypeFromToken(FileToken) == mdtModule && RidFromToken(FileToken) < m_pModules->CountFiles()));
    ASSERT(dwScope == 0 || TypeFromToken(FileToken) == mdtAssemblyRef);

    // Initialize to empty values
    *ppImportScope = NULL;

    HRESULT hr = S_OK;
    CFile* file = NULL;
    if (TypeFromToken(FileToken) == mdtAssemblyRef) {
        // Import from another assembly (possibly a nested file)
        if (SUCCEEDED(hr = m_pImports->GetFile( FileToken, &file)) &&
            dwScope > 0) {
            CAssemblyFile *assembly = (CAssemblyFile*)file;
            file = NULL;
            hr = assembly->GetFile(dwScope - 1, &file);
        }
    } else if (TypeFromToken(FileToken) == mdtModule) {
        hr = m_pModules->GetFile( FileToken, &file);
    } else {
        // Import from this Assembly
        // NOTE: we don't allow importing from the manifest file!
        ASSERT( TypeFromToken(FileToken) == mdtFile);
        hr = m_pAssem->GetFile( FileToken, &file);
    }

    if (SUCCEEDED(hr)) {
        if ((*ppImportScope = file->GetImportScope()))
            (*ppImportScope)->AddRef(); // Give a copy to them, AddRef so they can release
    }
    return hr;
}

HRESULT CAsmLink::ImportTypes(mdAssembly AssemblyID, mdToken FileToken, DWORD dwScope, HALINKENUM* phEnum,
                       IMetaDataImport **ppImportScope, DWORD* pdwCountOfTypes)
{
    ASSERT(m_bInited && !m_bPreClosed);
    ASSERT(AssemblyID == TokenFromRid(mdtAssembly, 1) || AssemblyID == AssemblyIsUBM);
    ASSERT((TypeFromToken(FileToken) == mdtFile && RidFromToken(FileToken) < m_pAssem->CountFiles()) || 
        (TypeFromToken(FileToken) == mdtAssemblyRef && RidFromToken(FileToken) < m_pImports->CountFiles()) || 
        (TypeFromToken(FileToken) == mdtModule && RidFromToken(FileToken) < m_pModules->CountFiles()) ||
        FileToken == AssemblyID);
    ASSERT(dwScope == 0 || TypeFromToken(FileToken) == mdtAssemblyRef);

    // Initialize to empty values
    if (ppImportScope)
        *ppImportScope = NULL;
    if (pdwCountOfTypes)
        *pdwCountOfTypes = 0;
    *phEnum = (HALINKENUM)NULL;

    HRESULT hr = S_OK;
    TypeEnumerator *TypeEnum = new TypeEnumerator;
    if (TypeEnum == NULL)
        return E_OUTOFMEMORY;

    TypeEnum->TypeID = 0;
    if (TypeFromToken(FileToken) == mdtAssemblyRef) {
        // Import from another assembly (possibly a nested file)
        if (SUCCEEDED(hr = m_pImports->GetFile( FileToken, &TypeEnum->file)) &&
            dwScope > 0) {
            CAssemblyFile *assembly = (CAssemblyFile*)TypeEnum->file;
            TypeEnum->file = NULL;
            hr = assembly->GetFile(dwScope - 1, &TypeEnum->file);
        }
    } else if (TypeFromToken(FileToken) == mdtModule) {
        hr = m_pModules->GetFile( FileToken, &TypeEnum->file);
    } else if (TypeFromToken(FileToken) == mdtFile){
        // Import from this Assembly
        hr = m_pAssem->GetFile( FileToken, &TypeEnum->file);
    } else {
        TypeEnum->file = m_pAssem;
        hr = S_OK;
    }


    if (SUCCEEDED(hr) && SUCCEEDED(hr = TypeEnum->file->ImportFile(pdwCountOfTypes, NULL))) {
        if ((ppImportScope != NULL) && (*ppImportScope = TypeEnum->file->GetImportScope()))
            (*ppImportScope)->AddRef(); // Give a copy to them, AddRef so they can release
        if (pdwCountOfTypes)
            *pdwCountOfTypes = TypeEnum->file->CountTypes();
        *phEnum = (HALINKENUM)TypeEnum;
        if (hr == S_OK && TypeEnum->file->CountTypes() == 0)
            hr = S_FALSE;
    } else {
        delete TypeEnum;
    }
    return hr;
}

HRESULT CAsmLink::EnumImportTypes(HALINKENUM hEnum, DWORD dwMax, mdTypeDef aTypeDefs[], DWORD* pdwCount)
{
    TypeEnumerator *TypeEnum = (TypeEnumerator*)hEnum;

    ASSERT(m_bInited && !m_bPreClosed);

    // Initialize to empty values
    *pdwCount = 0;
    HRESULT hr = S_OK;
    DWORD index = 0;
    
    while (SUCCEEDED(hr) && TypeEnum->TypeID < TypeEnum->file->CountTypes() && index < dwMax) {
        if (SUCCEEDED(hr = TypeEnum->file->GetType( TypeEnum->TypeID, &aTypeDefs[index]))) {
            TypeEnum->TypeID++;
            index++;
        }
    }
    *pdwCount = index;

    // Return S_FALSE if there are no more types to enumerate
    if (hr == S_OK && TypeEnum->TypeID == TypeEnum->file->CountTypes())
        hr = S_FALSE;

    return hr;
}

HRESULT CAsmLink::EnumCustomAttributes(HALINKENUM hEnum, mdToken tkType, mdCustomAttribute rCustomValues[], ULONG cMax, ULONG *pcCustomValues)
{
    TypeEnumerator *TypeEnum = (TypeEnumerator*)hEnum;

    ASSERT(m_bInited && !m_bPreClosed);

    // Initialize to empty values
    if (pcCustomValues)
        *pcCustomValues = 0;
    HRESULT hr = S_OK;
    DWORD index = 0, count = 0;
    
    while (SUCCEEDED(hr) && TypeEnum->TypeID < TypeEnum->file->CountCAs() && index < cMax) {
        if (SUCCEEDED(hr = TypeEnum->file->GetCA( TypeEnum->TypeID, tkType, &rCustomValues[index]))) {
            if (hr == S_OK) {
                // S_FALSE means it didn't match the filter
                count++;
                TypeEnum->TypeID++;
            }
            index++;
        }
    }
    if (pcCustomValues)
        *pcCustomValues = count;

    // Return S_FALSE if there are no more types to enumerate
    if (hr == S_OK && TypeEnum->TypeID == TypeEnum->file->CountTypes())
        hr = S_FALSE;

    return hr;
}

HRESULT CAsmLink::CloseEnum(HALINKENUM hEnum)
{
    ASSERT(m_bInited && !m_bPreClosed);
    ((TypeEnumerator*)hEnum)->file = NULL;
    delete (TypeEnumerator*)hEnum;
    return S_OK;
}

HRESULT CAsmLink::ExportType(mdAssembly AssemblyID, mdToken FileToken, mdTypeDef TypeToken,
                      LPCWSTR pszTypename, DWORD dwFlags, mdExportedType* pType)
{
    ASSERT(m_bInited && !m_bPreClosed);
    ASSERT(AssemblyID == TokenFromRid(mdtAssembly, 1) || AssemblyID == AssemblyIsUBM);
    ASSERT(AssemblyID == FileToken || (RidFromToken(FileToken) < m_pAssem->CountFiles() && TypeFromToken(FileToken) == mdtFile));
    if (AssemblyID == AssemblyIsUBM || FileToken == AssemblyID)
        return S_FALSE;

    HRESULT hr;
    CFile *file = NULL;

    if (FAILED(hr = m_pAssem->GetFile(RidFromToken(FileToken), &file)))
        return hr;

    return m_pAssem->AddExportType( file->GetFileToken(), TypeToken, pszTypename, dwFlags, pType);
}

HRESULT CAsmLink::ExportNestedType(mdAssembly AssemblyID, mdToken FileToken, mdTypeDef TypeToken, 
                            mdExportedType ParentType, LPCWSTR pszTypename, DWORD dwFlags, mdExportedType* pType)
{
    ASSERT(m_bInited && !m_bPreClosed);
    ASSERT(AssemblyID == TokenFromRid(mdtAssembly, 1) || AssemblyID == AssemblyIsUBM);
    ASSERT(AssemblyID == FileToken || (RidFromToken(FileToken) < m_pAssem->CountFiles() && TypeFromToken(FileToken) == mdtFile));
    ASSERT(IsTdNested(dwFlags));
    if (AssemblyID == AssemblyIsUBM || FileToken == AssemblyID)
        return S_FALSE;

    return m_pAssem->AddExportType( ParentType, TypeToken, pszTypename, dwFlags, pType);
}

HRESULT CAsmLink::EmbedResource(mdAssembly AssemblyID, mdToken FileToken, LPCWSTR pszResourceName, DWORD dwOffset, DWORD dwFlags)
{
    ASSERT(m_bInited && !m_bPreClosed);
    ASSERT(AssemblyID == TokenFromRid(mdtAssembly, 1) || AssemblyID == AssemblyIsUBM);
    ASSERT((RidFromToken(FileToken) < m_pAssem->CountFiles() && TypeFromToken(FileToken) == mdtFile) ||
        (FileToken == AssemblyID));

    HRESULT hr = S_OK;
    CFile *file = NULL;
    if (AssemblyID == AssemblyIsUBM) {
        if (SUCCEEDED(hr = m_pAssem->GetFile(FileToken, &file)))
            hr = file->AddResource(mdTokenNil, pszResourceName, dwOffset, dwFlags);
    } else if (FileToken == AssemblyID) {
        hr = m_pAssem->AddResource(mdTokenNil, pszResourceName, dwOffset, dwFlags);
    } else {
        if (SUCCEEDED(hr = m_pAssem->GetFile(FileToken, &file)))
            hr = m_pAssem->AddResource(file->GetFileToken(), pszResourceName, dwOffset, dwFlags);
    }
    return hr;
}

HRESULT CAsmLink::LinkResource(mdAssembly AssemblyID, LPCWSTR pszFileName, LPCWSTR pszNewLocation, LPCWSTR pszResourceName, DWORD dwFlags)
{
    ASSERT(m_bInited && !m_bPreClosed);
    ASSERT(AssemblyID == TokenFromRid(mdtAssembly, 1) || AssemblyID == AssemblyIsUBM);

    if (wcslen(pszFileName) > MAX_PATH)
        return FileNameTooLong(pszFileName); // File name too long
    HRESULT hr = S_OK;
    if (AssemblyID == AssemblyIsUBM) {
        hr = E_INVALIDARG;
    } else {
        if (pszNewLocation == NULL || pszNewLocation[0] == 0)
            pszNewLocation = pszFileName;
        
        CFile *file = NULL;
        file = new CFile( pszNewLocation, (IMetaDataEmit*)NULL, m_pError, this);
        if (file != NULL && pszNewLocation != NULL && pszNewLocation[0] != 0 &&
            wcscmp(pszFileName, pszNewLocation) != 0) {
            if (FAILED(hr = file->SetSource(pszFileName))) {
                delete file;
                return hr;
            }
        }
        hr = m_pAssem->AddFile( file, ffContainsNoMetaData, NULL);
        if (SUCCEEDED(hr))
            hr = m_pAssem->AddResource(file->GetFileToken(), pszResourceName, 0, dwFlags);
        else
            delete file;
    }
    return hr;
}

HRESULT CAsmLink::GetResolutionScope(mdAssembly AssemblyID, mdToken FileToken, mdToken TargetFile, mdToken* pScope)
{
    ASSERT(m_bInited && !m_bPreClosed && m_pAssem);
    ASSERT(AssemblyID == TokenFromRid(mdtAssembly, 1) || AssemblyID == AssemblyIsUBM);
    ASSERT((RidFromToken(FileToken) < m_pAssem->CountFiles() && TypeFromToken(FileToken) == mdtFile) ||
        (FileToken == AssemblyID));
    ASSERT(pScope != NULL);

    if (FileToken == TargetFile) {
        *pScope = TokenFromRid(1, mdtModule);
        return S_OK;
    }

    HRESULT hr = S_OK;
    CFile *file = NULL;
    if (FileToken == AssemblyID) {
        file = m_pAssem;
    } else {
        if (FAILED(hr = m_pAssem->GetFile(FileToken, &file)))
            return hr;
    }

    if (TypeFromToken(TargetFile) == mdtFile) {
        // make a moduleref to target file in "file"
        CFile *target;
        if (FAILED(hr = m_pAssem->GetFile(TargetFile, &target)))
            return hr;
        hr = target->MakeModuleRef(file->GetEmitScope(), pScope);
    } else if (TypeFromToken(TargetFile) == mdtModule) {
        // make a moduleref to target file in "file"
        CFile *target;
        if (FAILED(hr = m_pModules->GetFile(TargetFile, &target)))
            return hr;
        hr = target->MakeModuleRef(file->GetEmitScope(), pScope);
    } else if (TypeFromToken(TargetFile) == mdtAssembly) {
        ASSERT(TargetFile != AssemblyIsUBM);
        // make a moduleref to the manifest file in "file"
        hr = m_pAssem->MakeModuleRef(file->GetEmitScope(), pScope);
    } else if (TypeFromToken(TargetFile) == mdtAssemblyRef) {
        // make an assembly ref in "file"
        CFile *target;
        if (FAILED(hr = m_pImports->GetFile(TargetFile, &target)))
            return hr;
        if (file == m_pAssem)
            // Special Case this so we don't have to re-QI for the AssemblyEmit interface
            hr = ((CAssemblyFile*)target)->MakeAssemblyRef(m_pAssem->GetEmitter(), pScope);
        else
            hr = ((CAssemblyFile*)target)->MakeAssemblyRef(file->GetEmitScope(), pScope);
    } else
        hr = E_INVALIDARG;

    return hr;
}



// Simple helper function to report an option error.
static HRESULT ReportOptionError(CFile * file, AssemblyOptions option, HRESULT hr)
{
    if (file == NULL)
        return hr;
    return file->ReportError(ERR_BadOptionValueHR, mdTokenNil, NULL, ErrorOptionName(option), file->ErrorHR(hr));
}


HRESULT CAsmLink::SetAssemblyProps(mdAssembly AssemblyID, mdToken FileToken, AssemblyOptions Option, VARIANT Value)
{
    ASSERT(m_bInited && !m_bPreClosed && m_pAssem && !m_bManifestEmitted);
    ASSERT(AssemblyID == TokenFromRid(mdtAssembly, 1) || AssemblyID == AssemblyIsUBM);
    ASSERT((RidFromToken(FileToken) < m_pAssem->CountFiles() && TypeFromToken(FileToken) == mdtFile) ||
        (FileToken == AssemblyID));

    HRESULT hr = S_OK;
    if (Option >= optLastAssemOption || OptionCAs[Option].flag & 0x40)
        return E_INVALIDARG;
    if (AssemblyID == AssemblyIsUBM || (OptionCAs[Option].flag & 0x02)) {
        CFile *file = NULL;
        if (FileToken == AssemblyID)
            file = m_pAssem;
        else if (FAILED(hr = m_pAssem->GetFile(FileToken, &file)))
            return hr;

        ASSERT(file->GetEmitScope());
        IMetaDataEmit* pEmit = file->GetEmitScope();
        CComPtr<IMetaDataImport> pImport;
        mdToken tkAttrib = mdTokenNil, tkCtor;
        DWORD cbValue = 0, cbSig = 4;
        BYTE pbValue[2048];
        PBYTE pBlob = pbValue;
        COR_SIGNATURE newSig[9];
        LPCWSTR wszStr = NULL;
        ULONG wLen = 0;

        if (FAILED(hr = pEmit->QueryInterface(IID_IMetaDataImport, (void**)&pImport)))
            return hr;

        // Find or Create the TypeRef (This always scopes it to MSCORLIB)
        if (FAILED(hr = file->GetTypeRef(OptionCAs[Option].name, &tkAttrib)))
            return hr;

        // Make the Blob
        newSig[0] = (IMAGE_CEE_CS_CALLCONV_DEFAULT | IMAGE_CEE_CS_CALLCONV_HASTHIS);
        newSig[1] = 1; // One parameter
        newSig[2] = ELEMENT_TYPE_VOID;
        *(WORD*)pBlob = VAL16(1); // This is aligned
        pBlob += sizeof(WORD);

        if (V_VT(&Value) != OptionCAs[Option].vt)
            return E_INVALIDARG;
        switch(OptionCAs[Option].vt) {
        case VT_BOOL:
            *pBlob++ = (V_BOOL(&Value) == VARIANT_TRUE);
            newSig[3] = ELEMENT_TYPE_BOOLEAN;
            break;
        case VT_UI4:
            SET_UNALIGNED_VAL32(pBlob, V_UI4(&Value));
            pBlob += sizeof(ULONG);
            newSig[3] = ELEMENT_TYPE_U4;
            break;
        case VT_BSTR:
            if (Option == optAssemOS) {
                LPWSTR end = NULL;
                mdToken tkPlatform = mdTokenNil;
                newSig[1] = 2; // Two parameters
                newSig[3] = ELEMENT_TYPE_VALUETYPE;

                // Make the TypeRef
                if (FAILED(hr = file->GetTypeRef( PLATFORMID_NAME, &tkPlatform)))
                     break;

                cbSig = 5 + CorSigCompressToken(tkPlatform, newSig + 4);
                newSig[cbSig - 1] = ELEMENT_TYPE_STRING;
                SET_UNALIGNED_VAL32(pBlob, wcstoul(V_BSTR(&Value), &end, 0)); // Parse Hex, Octal, and Decimal
                pBlob += sizeof(ULONG);
                if (*end == L'.') {
                    wszStr = end++;
                    wLen = SysStringLen(V_BSTR(&Value)) - (UINT)(V_BSTR(&Value) - end);
                    goto ADDSTRING;
                } else {
                    hr = file->ReportError(ERR_InvalidOSString);
                    return hr;
                }
            } else {
                newSig[3] = ELEMENT_TYPE_STRING;
                wLen = SysStringLen(V_BSTR(&Value));
                wszStr = V_BSTR(&Value);
ADDSTRING:
                if (wLen == 0) {
                    // Too small for unilib
                    *pBlob++ = 0xFF;
                } else if (wLen & 0x80000000) {
                    // Too big!
                    return ReportOptionError(file, Option, E_INVALIDARG);
                } else if ((OptionCAs[Option].flag & 0x10) && wLen > MAX_PATH) {
                    // Too big!
                    return ReportOptionError(file, Option, HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW)); // File name too long
                } else {
                    CHAR pUTF8[2048];
                    int iLen = wLen;
    
                    wLen = (UINT)UnicodeToUTF8(wszStr, &iLen, pUTF8, lengthof(pUTF8));

                    iLen = (int)CorSigCompressData( wLen, pBlob);
                    pBlob += iLen;
                    if (wLen > (UINT)(pbValue + lengthof(pbValue) - pBlob)) {
                        // Too big!
                        return ReportOptionError(file, Option, HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW));
                    }
                    memcpy(pBlob, pUTF8, wLen);
                    pBlob += wLen;
                }
            }
            break;
        default:
            VSFAIL("Unknown Option Type!");
            newSig[3] = ELEMENT_TYPE_OBJECT;
            break;
        }
        hr = pImport->FindMemberRef(tkAttrib, L".ctor", newSig, cbSig, &tkCtor);
        if ((hr == CLDB_E_RECORD_NOTFOUND && FAILED(hr = pEmit->DefineMemberRef(tkAttrib, L".ctor", newSig, 4, &tkCtor))) ||
            FAILED(hr))
            return hr;
        cbValue = (DWORD)(pBlob - pbValue);

        // Emit the CA
        // This will also set the option if appropriate
        hr = EmitAssemblyCustomAttribute( AssemblyID, FileToken, tkCtor, pbValue, cbValue, 
            (OptionCAs[Option].flag & 0x08) ? TRUE : FALSE,
            (OptionCAs[Option].flag & 0x04) ? TRUE : FALSE);
    } else {
        // An assembly level custom attribute
        hr = m_pAssem->SetOption(Option, &Value);
    }
    return hr;
}

HRESULT CAsmLink::EmitAssemblyCustomAttribute(mdAssembly AssemblyID, mdToken FileToken, mdToken tkType,
                                       void const* pCustomValue, DWORD cbCustomValue, BOOL bSecurity, BOOL bAllowMultiple)
{
    ASSERT(m_bInited && !m_bPreClosed && !m_bManifestEmitted);
    ASSERT(AssemblyID == TokenFromRid(mdtAssembly, 1) || AssemblyID == AssemblyIsUBM);
    ASSERT((RidFromToken(FileToken) < m_pAssem->CountFiles() && TypeFromToken(FileToken) == mdtFile) ||
        (FileToken == AssemblyID));

    HRESULT hr = S_OK;
    if (AssemblyID == AssemblyIsUBM) {
        CFile *file = NULL;
        if (FAILED(hr = m_pAssem->GetFile(FileToken, &file)))
            return hr;
        hr = file->AddCustomAttribute(tkType, pCustomValue, cbCustomValue, bSecurity, bAllowMultiple, NULL);
    } else if (FileToken == AssemblyID) {
        hr = m_pAssem->AddCustomAttribute(tkType, pCustomValue, cbCustomValue, bSecurity, bAllowMultiple, NULL);
    } else {
        // An assembly level custom attribute that is in the wrong scope
        CFile *file = NULL;
        if (FAILED(hr = m_pAssem->GetFile(FileToken, &file)))
            return hr;
        hr = m_pAssem->AddCustomAttribute(tkType, pCustomValue, cbCustomValue, bSecurity, bAllowMultiple, file);
    }
    return hr;
}

HRESULT CAsmLink::EndMerge(mdAssembly AssemblyID)
{
    ASSERT(m_bInited && !m_bPreClosed && !m_bManifestEmitted);
    ASSERT(AssemblyID == TokenFromRid(mdtAssembly, 1));

    return m_pAssem->ImportCAs(m_pAssem);
}

HRESULT CAsmLink::EmitManifest(mdAssembly AssemblyID, DWORD* pdwReserveSize, mdAssembly* ptkManifest)
{
    ASSERT(m_bInited && !m_bPreClosed && !m_bManifestEmitted);
    ASSERT(AssemblyID == TokenFromRid(mdtAssembly, 1));
    m_bManifestEmitted = true;

    return m_pAssem->EmitManifest(pdwReserveSize, ptkManifest);
}

HRESULT CAsmLink::PreCloseAssembly(mdAssembly AssemblyID)
{
    ASSERT(m_bInited && !m_bPreClosed);
    ASSERT(AssemblyID == TokenFromRid(mdtAssembly, 1));
    m_bPreClosed = true;
    m_bManifestEmitted = false;

    // Emit the Manifest, all the cached CAs, and the hashes of files
    HRESULT hr = m_pAssem->EmitAssembly(m_pDisp);
    m_pAssem->PreClose();
    return hr;
}

HRESULT CAsmLink::CloseAssembly(mdAssembly AssemblyID)
{
    ASSERT(m_bInited && (m_bPreClosed || AssemblyID == AssemblyIsUBM));
    m_bPreClosed = false;
    m_bManifestEmitted = false;
    HRESULT hr;

    if (AssemblyID == AssemblyIsUBM) {
        hr = S_FALSE;
    } else {
        // Sign the assembly
        hr = m_pAssem->SignAssembly();
    }

    delete m_pAssem;
    m_pAssem = NULL;
    return hr;

    // After this is done, should be able to re-use the interface
}

// Find MSCORLIB
CAssemblyFile *CAsmLink::GetStdLib()
{
    if (m_pStdLib)
        return m_pStdLib;

    // First search imports
    DWORD i, count;
    CFile *file = NULL;
    HRESULT hr;

    for (i = 0, count = m_pImports->CountFiles(); i < count; i++) {
        hr = m_pImports->GetFile(i, &file);
        if (FAILED(hr))
            break;
        if (file && file->IsStdLib())
            return (m_pStdLib = (CAssemblyFile*)file);
    }

    CComPtr<IMetaDataAssemblyImport> pAImport;
    WCHAR szFilename[MAX_PATH + MAX_PATH];
    mdFile tkFile = mdTokenNil;

    if (FAILED(m_pDisp->GetCORSystemDirectory(szFilename, lengthof(szFilename) - 12, &count)))
        return NULL;
    wcscpy(szFilename + count - 1, L"mscorlib.dll");
    if (FAILED(ImportFile( szFilename, NULL, FALSE, &tkFile, &pAImport, NULL)) || !pAImport)
        return NULL;
    ASSERT(TypeFromToken(tkFile) == mdtAssemblyRef);
    if (FAILED(m_pImports->GetFile(RidFromToken(tkFile), &file)))
        return NULL;
    if (file && file->IsStdLib())
        return (m_pStdLib = (CAssemblyFile*)file);

    return NULL;
}


