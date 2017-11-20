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
// File: pefile.cpp
//
// Routines for creating executable files.
// ===========================================================================

#include "stdafx.h"

#include "compiler.h"
#include <io.h>
#include <mscoree.h>

// Name of the DLL that implements the debug symbol writer.
#define SYMBOL_WRITER_DLL    MAKEDLLNAME_A("ildbsymbols")

/*
 * ANSI/UNICODE indifferent file opener
 * also gets the file size iff fileLen != NULL
 */
HANDLE OpenFileEx( LPCWSTR filename, DWORD *fileLen)
{
    HANDLE local;
    DWORD len;

    local = W_CreateFile( filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    if (local == INVALID_HANDLE_VALUE || !fileLen)
        return local;

    len = GetFileSize( local, NULL);
    if (len == 0xFFFFFFFF) {
        CloseHandle( local);
        *fileLen = (DWORD) -1;
        local = INVALID_HANDLE_VALUE;
    } else {
        *fileLen = len;
    }

    return local;
}

CPEFile::CPEFile()
{
    Init(NULL);
}

/*
 * This initializes all the class variables and prepares it for Beginning
 */
void CPEFile::Init(COMPILER *comp)
{
    compiler = comp;
    debugemit = NULL;
    metaemit = NULL;
    assememit = NULL;
    outfile = NULL;
    ceeFile = NULL;
    ceefilegen = NULL;
    ilSection = 0;
    rdataSection = 0;
    m_dwOffset = 0;
    m_pResBuffer = NULL;
    m_dwBaseResRva = 0x00000001;
    m_dwResSize = 0;
}

/*
 * Just a little cleanup
 */
void CPEFile::Term()
{
    if (debugemit) {
        debugemit->Release();
        debugemit = NULL;
        if (!compiler->IsIncrementalRebuild()) {
            // For non-incremental rebuilds, the symbol writter hoses the PDB
            // so just nuke it
            WCHAR filename[MAX_PATH];
            GetPDBFileName(outfile, filename);
            W_DeleteFile(filename);
        }
    }
    if (metaemit) {
        metaemit->Release();
        metaemit = NULL;
    }
    if (assememit) {
        assememit->Release();
        assememit = NULL;
    }
    if (ceeFile) {
        ceefilegen->DestroyCeeFile(&ceeFile);
        ceeFile = NULL;
    }
    if (ceefilegen) {
        compiler->DestroyCeeFileGen(ceefilegen);
        ceefilegen = NULL;
    }

}

/*
 * Get the name of the PDB file associates with a give output file.
 */
void CPEFile::GetPDBFileName(OUTFILESYM * pOutFile, LPWSTR filename)
{
    wcscpy(filename, pOutFile->name->text);
    ReplaceFileExtension(filename, L".PDB");
}


/*
 * Create the debug symbol writer, if possible.
 */
typedef  HRESULT ( WINAPI * PFN_GETCLASSOBJECT)(REFCLSID, REFIID, LPVOID *);

HRESULT CPEFile::CreateSymbolWriter(ISymUnmanagedWriter ** pSymwriter)
{
    HRESULT hr;
    HMODULE hModule;
    PFN_GETCLASSOBJECT pfnGetClassObject;
    
    // First, try to load the symbol writer DLL manually and get the object. This insulates us from
    // registry goofiness.
    hModule = FindAndLoadHelperLibrary(SYMBOL_WRITER_DLL);
    if (hModule != NULL) {
        pfnGetClassObject = (PFN_GETCLASSOBJECT) GetProcAddress(hModule, "DllGetClassObject");
        if (pfnGetClassObject) {
            IClassFactory * pClassFactory;
            if (SUCCEEDED(pfnGetClassObject(CLSID_CorSymWriter, IID_IClassFactory, (LPVOID *) &pClassFactory))) {
                hr = pClassFactory->CreateInstance(NULL, IID_ISymUnmanagedWriter, (LPVOID *) pSymwriter);
                    
                pClassFactory->Release();
                if (SUCCEEDED(hr))
                    return hr;
            }
        }
    }

    // If that didn't work, just use CoCreateInstance.
    hr = CoCreateInstance(CLSID_CorSymWriter,
                           NULL,
                           CLSCTX_INPROC_SERVER,
                           IID_ISymUnmanagedWriter,
                           (void**)pSymwriter);
    return hr;
}

HRESULT CPEFile::CreateSymbolReader(ISymUnmanagedReader ** pSymreader)
{
    HRESULT hr;
    HMODULE hModule;
    PFN_GETCLASSOBJECT pfnGetClassObject;
    
    // First, try to load the symbol writer DLL manually and get the object. This insulates us from
    // registry goofiness.
    hModule = FindAndLoadHelperLibrary(SYMBOL_WRITER_DLL);
    if (hModule != NULL) {
        pfnGetClassObject = (PFN_GETCLASSOBJECT) GetProcAddress(hModule, "DllGetClassObject");
        if (pfnGetClassObject) {
            IClassFactory * pClassFactory;
            if (SUCCEEDED(pfnGetClassObject(CLSID_CorSymReader, IID_IClassFactory, (LPVOID *) &pClassFactory))) {
                hr = pClassFactory->CreateInstance(NULL, IID_ISymUnmanagedReader, (LPVOID *) pSymreader);
                    
                pClassFactory->Release();
                if (SUCCEEDED(hr))
                    return hr;
            }
        }
    }
    // If that didn't work, just use CoCreateInstance.
    hr = CoCreateInstance(CLSID_CorSymReader,
                           NULL,
                           CLSCTX_INPROC_SERVER,
                           IID_ISymUnmanagedReader,
                           (void**)pSymreader);
    return hr;
}


/*
 * Begin emitting an output file. If an error occurs, it is reported to the user
 * and then false is returned.
 */
bool CPEFile::BeginOutputFile(POUTFILESYM pOutFile)
{
    WCHAR filename[MAX_PATH];
    HRESULT hr;

    // Make sure an output file isn't already open.
    if (metaemit) {
        ASSERT(0);  // shouldn't happen.
        EndOutputFile(false);
    }

    // force a filename
    if (wcscmp(pOutFile->name->text, L"?") == 0)
        compiler->symmgr.SetOutFileName(pOutFile->firstInfile());
    ASSERT (pOutFile->name->text[0] != L'?' && pOutFile->name->text[0] != L'*');

    ceefilegen = compiler->CreateCeeFileGen();

    // Get the HCEEFILE and the important sections in the file.
    TimerStart(TIME_ICFG_CREATECEEFILE);
    CheckHR(ceefilegen->CreateCeeFile(&ceeFile));
    TimerStop(TIME_ICFG_CREATECEEFILE);

    TimerStart(TIME_ICFG_GETILSECTION);
    CheckHR(ceefilegen->GetIlSection(ceeFile, &ilSection));
    TimerStop(TIME_ICFG_GETILSECTION);

    TimerStart(TIME_ICFG_GETRDATASECTION);
    CheckHR(ceefilegen->GetRdataSection(ceeFile, &rdataSection));
    TimerStop(TIME_ICFG_GETRDATASECTION);

    if (compiler->IsIncrementalRebuild()) {
        //
        // grab the emit interface from the already open import interface
        //
        CheckHR(pOutFile->metaimport->QueryInterface(IID_IMetaDataEmit, (void**)&metaemit));
    } else {
        //
        // Get a new metadata scope to emit to, and connect with the ceefilegen.
        //
        IMetaDataDispenser * dispenser;

        dispenser = compiler->GetMetadataDispenser();

        TimerStart(TIME_IMD_DEFINESCOPE);
        CheckHR(dispenser->DefineScope(CLSID_CorMetaDataRuntime,  // Format of metadata
                                       0,                         // flags
                                       IID_IMetaDataEmit,         // Emitting interface
                                       (IUnknown **) &metaemit));
        TimerStop(TIME_IMD_DEFINESCOPE);
    }

    if (compiler->BuildAssembly() && FAILED(hr = metaemit->QueryInterface(IID_IMetaDataAssemblyEmit, (void**)&assememit)))
    {
        // Better error message.
        compiler->Error(NULL, FTL_ComPlusInit, compiler->ErrHR(hr));
    }

    if (compiler->options.m_fEMITDEBUGINFO) {
        if (FAILED(hr = CreateSymbolWriter(&debugemit)))
        {
            // Better error message.
            compiler->Error(NULL, FTL_DebugInit, compiler->ErrHR(hr));
        }


        // Create the PDB filename
        GetPDBFileName(pOutFile, filename);
        // create stream for EnC
        ASSERT(!symbolStream);

        // Initialize debugger.
        hr = debugemit->Initialize(metaemit, filename, symbolStream, !compiler->IsIncrementalRebuild());
        if (FAILED(hr)) {
            if (hr == HRESULT_FROM_WIN32(ERROR_BAD_FORMAT)) 
                compiler->Error(NULL, FTL_BadPDBFormat, filename);
            else
                compiler->Error(NULL, FTL_DebugInitFile, filename, compiler->ErrHR(hr));
        }
    }

    // Set the module properties
    CheckHR(metaemit->SetModuleProps(GetModuleName(pOutFile, compiler)));

    // Remember output file
    outfile = pOutFile;

    return true;
}

/*static*/
LPCWSTR CPEFile::GetModuleName(POUTFILESYM pOutFile, COMPILER * compiler)
{
    WCHAR baseFileName[_MAX_FNAME + _MAX_EXT + 1];
    WCHAR ext[_MAX_EXT];

    if (compiler->options.moduleName) {
        return compiler->options.moduleName;
    } else {
        // force a filename
        if (wcscmp(pOutFile->name->text, L"?") == 0)
            compiler->symmgr.SetOutFileName(pOutFile->firstInfile());
        ASSERT (pOutFile->name->text[0] != L'?' && pOutFile->name->text[0] != L'*');

        _wsplitpath(pOutFile->name->text, NULL, NULL, baseFileName, ext);
        wcscat(baseFileName, ext);

        return compiler->globalSymAlloc.AllocStr(baseFileName);
    }
}

void CPEFile::SetAttributes(bool fDll)
{
    // Set output file attributes.
    TimerStart(TIME_ICFG_SETDLLSWITCH);
    CheckHR(ceefilegen->SetDllSwitch(ceeFile, fDll));
    TimerStop(TIME_ICFG_SETDLLSWITCH);

    TimerStart(TIME_ICFG_SETENTRYPOINT);
    CheckHR(ceefilegen->SetEntryPoint(ceeFile,
        outfile->entrySym ? outfile->entrySym->tokenEmit : mdTokenNil));
    TimerStop(TIME_ICFG_SETENTRYPOINT);

    if (outfile->entrySym && compiler->options.m_fEMITDEBUGINFO) {
        TimerStart(TIME_ISW_SETUSERENTRYPOINT);
        CheckHR(FTL_DebugEmitFailure, debugemit->SetUserEntryPoint(outfile->entrySym->tokenEmit));
        TimerStop(TIME_ISW_SETUSERENTRYPOINT);
    }
}

void *CPEFile::GetCodeLocationOfOffset(ULONG offset)
{
    void *ptr = 0;
    TimerStart(TIME_ICFG_COMPUTESECTIONPOINTER);
    CheckHR(ceefilegen->ComputeSectionPointer(ilSection, offset, (char**) &ptr));
    TimerStop(TIME_ICFG_COMPUTESECTIONPOINTER);
    return ptr;
}

ULONG CPEFile::GetRVAOfOffset(ULONG offset)
{
    ULONG codeRVA;
    TimerStart(TIME_ICFG_GETMETHODRVA);
    CheckHR(ceefilegen->GetMethodRVA(ceeFile, offset, &codeRVA));
    TimerStop(TIME_ICFG_GETMETHODRVA);

    return codeRVA;
}

void *CPEFile::AllocateRVABlock(ULONG cb, ULONG alignment, ULONG *codeRVA, ULONG *offset)
{
    void * codeLocation;

    // Only call this if you are really emitting code
    ASSERT(cb > 0);

    // Get a block of size cb
    ULONG offsetCur;

    TimerStart(TIME_ICFG_GETSECTIONBLOCK);
    CheckHR(ceefilegen->GetSectionBlock(ilSection, cb, alignment, & codeLocation));
    TimerStop(TIME_ICFG_GETSECTIONBLOCK);

    TimerStart(TIME_ICFG_GETSECTIONDATALEN);
    CheckHR(ceefilegen->GetSectionDataLen(ilSection, &offsetCur));
    TimerStop(TIME_ICFG_GETSECTIONDATALEN);

    offsetCur -= cb;
    lastMethodLoc = offsetCur;
    *codeRVA = GetRVAOfOffset(offsetCur);
    *offset = (int) offsetCur;

    // Return the block of code allocated for this method.
    return codeLocation;
}

/*
 * Record the fact that a string token lives at the offset within
 * the last returned code block. This is used as a fix-up notification.
 */
void CPEFile::DefineStringLoc(int offset)
{
    TimerStart(TIME_ICFG_ADDSECTIONRELOC);
    CheckHR(ceefilegen->AddSectionReloc(ilSection, lastMethodLoc + offset,
        rdataSection, srRelocAbsolute));
    TimerStop(TIME_ICFG_ADDSECTIONRELOC);
}

void CPEFile::WriteCryptoKey()
{
    DWORD dwKeySize = 0;
    HRESULT hr;

    TimerStart(TIME_SN_SIGNATUREGENERATION);
    hr = compiler->linker->EmitManifest(compiler->assemID, &dwKeySize, NULL);
    TimerStop(TIME_SN_SIGNATUREGENERATION);

    if (FAILED(hr)) {
        compiler->Error(NULL, ERR_CryptoFailed, 
            compiler->ErrHR(hr), outfile->name->text);
    } else if (hr == S_OK && dwKeySize > 0) {
        DWORD dwKeyOffset = 0, dwKeyRVA = 0;
        PBYTE pbBuffer = NULL;

        pbBuffer = (PBYTE) compiler->AllocateRVABlock(0, dwKeySize, 1, &dwKeyRVA, &dwKeyOffset);
        memset(pbBuffer, 0, dwKeySize); // Zero it out

        TimerStart(TIME_ICFG_SETSTRONGNAMEENTRY);
        CheckHR(ceefilegen->SetStrongNameEntry( ceeFile, dwKeySize, dwKeyRVA));
        TimerStop(TIME_ICFG_SETSTRONGNAMEENTRY);
    }

}

/*
 * Get the size of embedded resources so we can emit them together
 */
bool CPEFile::CalcResource(PRESFILESYM pRes)
{
    if (!pRes->isEmbedded)
        return true;

    ASSERT(m_pResBuffer == NULL && m_dwOffset == 0); // Can't add a resource after we've emitted some

    DWORD fileLen = 0;
    HANDLE hFile = OpenFileEx( pRes->filename, &fileLen);
    if (hFile == INVALID_HANDLE_VALUE) {
        compiler->Error( NULL, ERR_CantReadResource, pRes->filename, compiler->ErrGetLastError());
        return false;
    }
    CloseHandle(hFile);
    // Add up the size (with padding)
    m_dwResSize = ((m_dwResSize + 0x3) & 0xFFFFFFFC) + fileLen + sizeof(DWORD);

    return true;
}

bool CPEFile::AllocResourceBlob()
{
    ASSERT(m_pResBuffer == NULL && m_dwOffset == 0);
    if (m_dwResSize == 0)
        return true;

    ASSERT(m_dwBaseResRva == 0x00000001);
    DWORD offsetResource;
    m_pResBuffer = (PBYTE) compiler->AllocateRVABlock(0, m_dwResSize, 4, &m_dwBaseResRva, &offsetResource);
        
    return (m_pResBuffer != NULL);
}

/*
 * Adds a resource to the Assembly Manifest
 * If pRes->isEmbedded is true, then it will embed the resource within this assembly's PE file
 *
 */
bool CPEFile::AddResource(PRESFILESYM pRes)
{
    ASSERT(metaemit);

    if (pRes->isEmbedded) {
        HANDLE hFile;
        DWORD fileLen = 0, dwRead;

        ASSERT(m_pResBuffer != NULL && m_dwResSize > 0);
        DWORD dwOffset = (m_dwOffset + 0x03) & 0xFFFFFFFC; // Pad
        ASSERT(dwOffset < m_dwResSize);
        BYTE * pbBuffer = ((BYTE*)m_pResBuffer) + dwOffset;

        hFile = OpenFileEx( pRes->filename, &fileLen);
        if (hFile == INVALID_HANDLE_VALUE) {
            compiler->Error( NULL, ERR_CantReadResource, pRes->filename, compiler->ErrGetLastError());
            return false;
        }

        m_dwOffset = dwOffset + fileLen + sizeof(DWORD);
        ASSERT(m_dwOffset <= m_dwResSize);
        *(DWORD*)pbBuffer = VAL32(fileLen);
        pbBuffer += sizeof(DWORD);
        if (!ReadFile( hFile, pbBuffer, fileLen, &dwRead, NULL)) {
            compiler->Error( NULL, ERR_CantReadResource, pRes->filename, compiler->ErrGetLastError());
            CloseHandle( hFile);
            return false;
        }
        CloseHandle( hFile);

        TimerStart(TIME_IMAE_DEFINEMANIFESTRESOURCE);
        CheckHR(compiler->linker->EmbedResource(compiler->assemID, compiler->assemFile.outfile->idFile, pRes->name->text, 
            dwOffset, pRes->isVis ? mrPublic : mrPrivate));
        TimerStop(TIME_IMAE_DEFINEMANIFESTRESOURCE);
    } else {
        if (compiler->BuildAssembly()) {
            TimerStart(TIME_IMAE_DEFINEMANIFESTRESOURCE);
            CheckHR(compiler->linker->LinkResource(compiler->assemID, pRes->filename, NULL, pRes->name->text, 
                pRes->isVis ? mrPublic : mrPrivate));
            TimerStop(TIME_IMAE_DEFINEMANIFESTRESOURCE);
        } else {
            compiler->Error(NULL, ERR_CantRefResource, pRes->name->text);
            return false;
        }
    }

    return true;
}


#define WToMB(pwstr, cchW, pstr, cchMB) WideCharToMultiByte(CP_ACP, 0, pwstr, cchW, pstr, cchMB, 0, 0)

//--------------------------------------------------------------
BOOL WINAPI W_MoveFile (PCWSTR pSourceFileName, PCWSTR pDestFileName)
{
    if (!MoveFileW(pSourceFileName, pDestFileName)) {

        return false;
    }

    return true;
}

/*
 * End writing an output file. If true is passed, the output file is actually written.
 * If false is passed (e.g., because an error occurred), the output file is not
 * written.
 *
 * true is returned if the output file was successfully written.
 */
bool CPEFile::EndOutputFile(bool writeFile)
{
    bool useTempOutFile = false;
    bool useTempResFile = false;
    WCHAR outFileName[MAX_PATH+1];
    WCHAR resFileName[MAX_PATH+1];
    HRESULT hr;

    PAL_TRY {

        if (writeFile && outfile->name->text[0] != L'*') {
            // Set output file attributes.
            if (compiler->IsIncrementalRebuild()) {
                WCHAR temppath[MAX_PATH+1];
                if (0 == W_GetTempPath(MAX_PATH, temppath)) {
                    CheckHR( HRESULT_FROM_WIN32(GetLastError()));
                    goto FAILED;
                }
                if (0 == W_GetTempFileName(temppath, L"CSC", 0, outFileName)) {
                    CheckHR( HRESULT_FROM_WIN32(GetLastError()));
                    goto FAILED;
                }
                useTempOutFile = true;
            } else {
                ASSERT (outfile->name->text[0] != L'?' && outfile->name->text[0] != L'*');
                wcscpy(outFileName, outfile->name->text);
            }

            //
            // release the import interface so that we can re-write the file
            //
            if (outfile->metaimport) {
                outfile->metaimport->Release();
                outfile->metaimport = NULL;
            }

            if (!compiler->inEncMode()) {
                TimerStart(TIME_ICFG_SETOUTPUTFILENAME);
                CheckHR(ceefilegen->SetOutputFileName(ceeFile, outFileName));
                TimerStop(TIME_ICFG_SETOUTPUTFILENAME);
            }

            TimerStart(TIME_ICFG_SETCOMIMAGEFLAGS);
            CheckHR(ceefilegen->SetComImageFlags(ceeFile, COMIMAGE_FLAGS_ILONLY));
            TimerStop(TIME_ICFG_SETCOMIMAGEFLAGS);

            if (outfile->imageBase != 0) {
                TimerStart(TIME_ICFG_SETIMAGEBASE);
                CheckHR(ceefilegen->SetImageBase(ceeFile, outfile->imageBase));
                TimerStop(TIME_ICFG_SETIMAGEBASE);
            }

            if (outfile->fileAlign != 0) {
                TimerStart(TIME_ICFG_SETFILEALIGNMENT);
                CheckHR(ceefilegen->SetFileAlignment(ceeFile, outfile->fileAlign));
                TimerStop(TIME_ICFG_SETFILEALIGNMENT);
            }

            // this is actually for resources
            if (m_dwResSize > 0) {
                TimerStart(TIME_ICFG_SETMANIFESTENTRY);
                CheckHR(ceefilegen->SetManifestEntry(ceeFile, m_dwResSize, m_dwBaseResRva));
                TimerStop(TIME_ICFG_SETMANIFESTENTRY);
            }


            if (compiler->ErrorCount())
                goto FAILED;

            if (compiler->options.m_fEMITDEBUGINFO) {
                DWORD cbCVInfo;
                BYTE * pbCVInfo;
                ULONG offset;

                // Get size of path information to PDB file.
                TimerStart(TIME_ISW_GETDEBUGCVINFO);
                CheckHR(FTL_DebugEmitFailure, debugemit->GetDebugInfo(NULL, 0, &cbCVInfo, NULL));
                TimerStop(TIME_ISW_GETDEBUGCVINFO);

                if (cbCVInfo > 0) {
                    // Allocate space to store it.
                    DWORD rva;
                    pbCVInfo = (BYTE*) compiler->AllocateRVABlock(0, sizeof(IMAGE_DEBUG_DIRECTORY) + cbCVInfo, 4, &rva, &offset);

                    // Write it into the PE
                    TimerStart(TIME_ISW_GETDEBUGCVINFO);
                    CheckHR(FTL_DebugEmitFailure, debugemit->GetDebugInfo((IMAGE_DEBUG_DIRECTORY *)pbCVInfo, cbCVInfo, &cbCVInfo, pbCVInfo + sizeof(IMAGE_DEBUG_DIRECTORY)));
                    TimerStop(TIME_ISW_GETDEBUGCVINFO);

                    ((IMAGE_DEBUG_DIRECTORY *) pbCVInfo)->AddressOfRawData = VAL32(offset + sizeof(IMAGE_DEBUG_DIRECTORY));
                    ((IMAGE_DEBUG_DIRECTORY *) pbCVInfo)->PointerToRawData = VAL32(offset + sizeof(IMAGE_DEBUG_DIRECTORY));
                    time_t timeStamp;
                    CheckHR(ceefilegen->GetFileTimeStamp(ceeFile, &timeStamp));
                    ((IMAGE_DEBUG_DIRECTORY *) pbCVInfo)->TimeDateStamp = VAL32((DWORD)timeStamp);

                    TimerStart(TIME_ICFG_ADDSECTIONRELOC);
                    CheckHR(ceefilegen->AddSectionReloc(ilSection, offset + offsetof(IMAGE_DEBUG_DIRECTORY, AddressOfRawData),
                                                        ilSection, srRelocAbsolute));
                    CheckHR(ceefilegen->AddSectionReloc(ilSection, offset + offsetof(IMAGE_DEBUG_DIRECTORY, PointerToRawData),
                                                        ilSection, srRelocFilePos));
                    TimerStop(TIME_ICFG_ADDSECTIONRELOC);

                    // Set the directory entry.
                    TimerStart(TIME_ICFG_SETDIRECTORYENTRY);
                    CheckHR(ceefilegen->SetDirectoryEntry(ceeFile, ilSection, IMAGE_DIRECTORY_ENTRY_DEBUG, sizeof(IMAGE_DEBUG_DIRECTORY), offset));
                    TimerStop(TIME_ICFG_SETDIRECTORYENTRY);
                }

                CheckHR(FTL_DebugEmitFailure, debugemit->Close());
                debugemit->Release();
                debugemit = NULL;

                if (compiler->ErrorCount())
                    goto FAILED;

                {
                    ASSERT(!symbolStream);
                }
            }

            TimerStart(TIME_ICFG_SETSUBSYSTEM);
            CheckHR(ceefilegen->SetSubsystem(ceeFile, outfile->isConsoleApp ? IMAGE_SUBSYSTEM_WINDOWS_CUI : IMAGE_SUBSYSTEM_WINDOWS_GUI, 4, 0));
            TimerStop(TIME_ICFG_SETSUBSYSTEM);

            // Write the output file and metadata.
            TimerStart(TIME_ICFG_EMITMETADATAEX);
            CheckHR(ceefilegen->EmitMetaDataEx(ceeFile, metaemit));
            TimerStop(TIME_ICFG_EMITMETADATAEX);

            if (compiler->ErrorCount())
                goto FAILED;

            if (!compiler->inEncMode()) {

                ASSERT(!symbolStream);

                TimerStart(TIME_ICFG_GENERATECEEFILE);
                hr = ceefilegen->GenerateCeeFile(ceeFile);
                TimerStop(TIME_ICFG_GENERATECEEFILE);
                    CheckHR(ERR_OutputWriteFailed, hr);

            } 
        }
FAILED:     // Goto here if we had errors
        ;
    }
    PAL_FINALLY {
        // This needs to always happen, even if writing failed.
        symbolStream = 0;
        if (debugemit) {
            debugemit->Release();
            debugemit = NULL;
            if (!compiler->IsIncrementalRebuild()) {
                // For non-incremental rebuilds, the symbol writter hoses the PDB
                // so just nuke it
                WCHAR filename[MAX_PATH];
                GetPDBFileName(outfile, filename);
                W_DeleteFile(filename);
            }
        }
        if (metaemit) {
            metaemit->Release();
            metaemit = NULL;
        }
        if (assememit) {
            assememit->Release();
            assememit = NULL;
        }
        if (ceeFile) {
            ceefilegen->DestroyCeeFile(&ceeFile);
            ceeFile = NULL;
        }
        if (ceefilegen) {
            compiler->DestroyCeeFileGen(ceefilegen);
            ceefilegen = NULL;
        }
        if (useTempOutFile && writeFile) {
            //
            // Delete the output file and replace it with the new file
            //
            if (!MoveFileExW(outFileName, outfile->name->text, MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING)) {
                if (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED) {
                    W_DeleteFile(outfile->name->text);
                    if (!W_MoveFile(outFileName, outfile->name->text)) {
                        CheckHR(HRESULT_FROM_WIN32(GetLastError()));
                    }
                } else {
                    CheckHR(HRESULT_FROM_WIN32(GetLastError()));
                }
            }

        }
        if (useTempResFile) {
            // Delete the auto-generated .RES file
            if (!W_DeleteFile(resFileName))
                compiler->Error(NULL, WRN_DeleteAutoResFailed, resFileName, compiler->ErrGetLastError());
        }

        outfile = NULL;
    }
    PAL_ENDTRY

    return writeFile ? true : false;  // Never return true if writeFile was false.
}


/*
 * Modifies lpszFilename by removing it's extension and replacing it
 * with the one specified
 */
void CPEFile::ReplaceFileExtension(LPWSTR lpszFilename, LPCWSTR lpszExt)
{
    const WCHAR *ext;

    // Extension is last '.' not followed by '\' or '/'
    // copy everything up to '.'
    ext = wcsrchr(lpszFilename, L'.');
    if (ext && wcschr(ext, L'/') == NULL && wcschr(ext, L'\\') == NULL) {
        lpszFilename[(DWORD)(ext-lpszFilename)] = L'\0';
    }

    // append extension
    wcscat(lpszFilename, lpszExt);
}

/*
 * Semi-generic method to check for failure of a meta-data method.
 */
__forceinline void CPEFile::CheckHR(HRESULT hr)
{
    if (FAILED(hr))
        MetadataFailure(hr);
    SetErrorInfo(0, NULL); // Clear out any stale ErrorInfos
}


/*
 * Semi-generic method to check for failure of a meta-data method. Passed
 * an error ID in cases where the generic one is no good.
 */
__forceinline void CPEFile::CheckHR(int errid, HRESULT hr)
{
    if (FAILED(hr))
        MetadataFailure(errid, hr);
    SetErrorInfo(0, NULL); // Clear out any stale ErrorInfos
}

/*
 * Handle a generic meta-data API failure.
 */
void CPEFile::MetadataFailure(HRESULT hr)
{
    MetadataFailure(FTL_MetadataEmitFailure, hr);
}

/*
 * Handle an API failure. The passed error code is expected to take one insertion string
 * that is filled in with the HRESULT.
 */
void CPEFile::MetadataFailure(int errid, HRESULT hr)
{
    compiler->Error(NULL, errid, compiler->ErrHR(hr), this->outfile ? this->outfile->name->text : L"");
}


/* static */
PVOID CPEFile::Cor_RtlImageRvaToVa(PVOID Base, ULONG Rva)
{
    IMAGE_NT_HEADERS32 *pNT32 = NULL;
    IMAGE_NT_HEADERS64 *pNT64 = NULL;

    FindNTHeader( (PBYTE)Base, &pNT32, &pNT64);
    ASSERT( !pNT32 || !pNT64);

    if (pNT32)
        return Cor_RtlImageRvaToVa(pNT32, Base, Rva);
    else if (pNT64)
        return Cor_RtlImageRvaToVa(pNT64, Base, Rva);
    else
        return NULL;
}

/* static */
PIMAGE_SECTION_HEADER CPEFile::Cor_RtlImageRvaToSection(PVOID Base, ULONG Rva)
{
    IMAGE_NT_HEADERS32 *pNT32 = NULL;
    IMAGE_NT_HEADERS64 *pNT64 = NULL;

    FindNTHeader( (PBYTE)Base, &pNT32, &pNT64);
    ASSERT( !pNT32 || !pNT64);

    if (pNT32)
        return Cor_RtlImageRvaToSection(pNT32, Base, Rva);
    else if (pNT64)
        return Cor_RtlImageRvaToSection(pNT64, Base, Rva);
    else
        return NULL;
}

/* static */
PIMAGE_SECTION_HEADER CPEFile::Cor_RtlImageRvaToSection(PIMAGE_NT_HEADERS32 NtHeaders, PVOID Base, ULONG Rva)
{
    ULONG i;
    PIMAGE_SECTION_HEADER NtSection;

    NtSection = IMAGE_FIRST_SECTION( NtHeaders );
    for (i=0; i<VAL16(NtHeaders->FileHeader.NumberOfSections); i++) {
        if (Rva >= VAL32(NtSection->VirtualAddress) &&
            Rva < VAL32(NtSection->VirtualAddress) + VAL32(NtSection->SizeOfRawData))
            return NtSection;
        
        ++NtSection;
    }

    return NULL;
}
/* static */
PIMAGE_SECTION_HEADER CPEFile::Cor_RtlImageRvaToSection(PIMAGE_NT_HEADERS64 NtHeaders, PVOID Base, ULONG Rva)
{
    ULONG i;
    PIMAGE_SECTION_HEADER NtSection;

    NtSection = IMAGE_FIRST_SECTION( NtHeaders );
    for (i=0; i<VAL16(NtHeaders->FileHeader.NumberOfSections); i++) {
        if (Rva >= VAL32(NtSection->VirtualAddress) &&
            Rva < VAL32(NtSection->VirtualAddress) + VAL32(NtSection->SizeOfRawData))
            return NtSection;
        
        ++NtSection;
    }

    return NULL;
}


/* static */
PVOID CPEFile::Cor_RtlImageRvaToVa(PIMAGE_NT_HEADERS32 NtHeaders, PVOID Base, ULONG Rva)
{
    PIMAGE_SECTION_HEADER NtSection = Cor_RtlImageRvaToSection(NtHeaders, Base,  Rva);

    if (NtSection != NULL) {
        return (PVOID)((PCHAR)Base +
                       (Rva - VAL32(NtSection->VirtualAddress)) +
                       VAL32(NtSection->PointerToRawData));
    }
    else
        return NULL;
}
/* static */
PVOID CPEFile::Cor_RtlImageRvaToVa(PIMAGE_NT_HEADERS64 NtHeaders, PVOID Base, ULONG Rva)
{
    PIMAGE_SECTION_HEADER NtSection = Cor_RtlImageRvaToSection(NtHeaders, Base,  Rva);

    if (NtSection != NULL) {
        return (PVOID)((PCHAR)Base +
                       (Rva - VAL32(NtSection->VirtualAddress)) +
                       VAL32(NtSection->PointerToRawData));
    }
    else
        return NULL;
}

/* static */
void CPEFile::FindNTHeader(PBYTE pbMapAddress, PIMAGE_NT_HEADERS32 *ppNT32, PIMAGE_NT_HEADERS64 *ppNT64)
{
    IMAGE_DOS_HEADER   *pDosHeader;

    *ppNT32 = NULL;
    *ppNT64 = NULL;
    pDosHeader = (IMAGE_DOS_HEADER *) pbMapAddress;

    if ((pDosHeader->e_magic == VAL16(IMAGE_DOS_SIGNATURE)) &&
        (pDosHeader->e_lfanew != 0) &&
        GET_UNALIGNED_32((VAL32(pDosHeader->e_lfanew) + (BYTE*)pDosHeader)) == 
        VAL32(IMAGE_NT_SIGNATURE))
    {
        IMAGE_NT_HEADERS32 *pNT = (IMAGE_NT_HEADERS32*)(VAL32(pDosHeader->e_lfanew) + (BYTE*)pDosHeader);
        // NOTE: this might not be a real 32-bit header, but the 64-bit and 32-bit headers
        // are the same up until the Magic number
        if (pNT->OptionalHeader.Magic == VAL16(IMAGE_NT_OPTIONAL_HDR32_MAGIC)) {
            *ppNT32 = pNT;

            if ((*ppNT32)->FileHeader.SizeOfOptionalHeader !=
                 VAL16(IMAGE_SIZEOF_NT_OPTIONAL32_HEADER))
                *ppNT32 = NULL;
        } else if (pNT->OptionalHeader.Magic == VAL16(IMAGE_NT_OPTIONAL_HDR64_MAGIC)) {
            *ppNT64 = (IMAGE_NT_HEADERS64*) (pNT);

            if ((*ppNT64)->FileHeader.SizeOfOptionalHeader !=
                VAL16(IMAGE_SIZEOF_NT_OPTIONAL64_HEADER))
                *ppNT64 = NULL;
        }
    }
}
