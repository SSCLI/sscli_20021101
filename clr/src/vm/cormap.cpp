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
//*****************************************************************************
// CorMap.cpp
//
// Implementation for mapping in files without using system services
//
//*****************************************************************************
#include "common.h"
#include "cormap.hpp"
#include "eeconfig.h"
#include "safegetfilesize.h"

LONG                    CorMap::m_spinLock = 0;
BOOL                    CorMap::m_fInitialized = FALSE;
CRITICAL_SECTION        CorMap::m_pCorMapCrst;
#ifdef _DEBUG
DWORD                   CorMap::m_fInsideMapLock = 0;
#endif

DWORD                   CorMap::m_dwIndex = 0;
DWORD                   CorMap::m_dwSize = 0;
EEUnicodeStringHashTable*  CorMap::m_pOpenFiles = NULL;

#define BLOCK_SIZE 20
#define BLOCK_NUMBER 20


HRESULT STDMETHODCALLTYPE RuntimeOpenImage(LPCWSTR pszFileName, HCORMODULE* hHandle)
{
    HRESULT hr;
    IfFailRet(CorMap::Attach());
    return CorMap::OpenFile(pszFileName, CorLoadOSMap, hHandle);
}

HRESULT STDMETHODCALLTYPE RuntimeOpenImageInternal(LPCWSTR pszFileName, HCORMODULE* hHandle, DWORD *pdwLength)
{
    HRESULT hr;
    IfFailRet(CorMap::Attach());
    return CorMap::OpenFile(pszFileName, CorLoadOSMap, hHandle, pdwLength);
}

HRESULT STDMETHODCALLTYPE RuntimeReleaseHandle(HCORMODULE hHandle)
{
    HRESULT hr;
    IfFailRet(CorMap::Attach());
    return CorMap::ReleaseHandle(hHandle);
}

HRESULT STDMETHODCALLTYPE RuntimeReadHeaders(PBYTE hAddress, IMAGE_DOS_HEADER** ppDos,
                                             IMAGE_NT_HEADERS** ppNT, IMAGE_COR20_HEADER** ppCor,
                                             BOOL fDataMap, DWORD dwLength)
{
    HRESULT hr;
    IfFailRet(CorMap::Attach());
    return CorMap::ReadHeaders(hAddress, ppDos, ppNT, ppCor, fDataMap, dwLength);
}

CorLoadFlags STDMETHODCALLTYPE RuntimeImageType(HCORMODULE hHandle)
{
    if(FAILED(CorMap::Attach()))
       return CorLoadUndefinedMap;
       
    return CorMap::ImageType(hHandle);
}

HRESULT STDMETHODCALLTYPE RuntimeOSHandle(HCORMODULE hHandle, HMODULE* hModule)
{
    HRESULT hr;
    IfFailRet(CorMap::Attach());
    return CorMap::BaseAddress(hHandle, hModule);
}

HRESULT CorMap::Attach()
{
    EnterSpinLock ();
    if(m_fInitialized == FALSE) {
        InitializeCriticalSection(&m_pCorMapCrst);
        m_fInitialized = TRUE;
    }
    LeaveSpinLock();
    return S_OK;
}



DWORD CorMap::CalculateCorMapInfoSize(DWORD dwFileName)
{
    // Align the value with the size of the architecture
    DWORD cbInfo = (dwFileName + 1) * sizeof(WCHAR) + sizeof(CorMapInfo);
    DWORD algn = MAX_NATURAL_ALIGNMENT - 1;
    cbInfo = (cbInfo + algn) & ~algn;
    return cbInfo;
}

HRESULT CorMap::LoadImage(HANDLE hFile, 
                          CorLoadFlags flags, 
                          PBYTE *hMapAddress, 
                          LPCWSTR pFileName, 
                          DWORD dwFileName)
{
    HRESULT hr = S_OK;
    if(flags == CorLoadOSImage) {
        DWORD cbInfo = CalculateCorMapInfoSize(dwFileName);
        PBYTE pMapInfo = SetImageName((PBYTE) NULL,
                                      cbInfo,
                                      pFileName, dwFileName,
                                      flags,
                                      0);
        if (!pMapInfo)
            hr = E_OUTOFMEMORY;
        else if(hMapAddress)
            *hMapAddress = pMapInfo;
    }
    else if(flags == CorLoadImageMap) {
        if(MapImageAsData(hFile, CorLoadDataMap, hMapAddress, pFileName, dwFileName))
            hr = LayoutImage(*hMapAddress, *hMapAddress);
        else {
            hr = HRESULT_FROM_WIN32(GetLastError());
            if (SUCCEEDED(hr))
                hr = HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
        }
    }
    else {
        if(!MapImageAsData(hFile, flags, hMapAddress, pFileName, dwFileName)) {
            hr = HRESULT_FROM_WIN32(GetLastError());
            if (SUCCEEDED(hr))
                hr = HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
        }
    }
    return hr;
}


PBYTE CorMap::SetImageName(PBYTE hMemory, 
                           DWORD cbInfo, 
                           LPCWSTR pFileName, 
                           DWORD dwFileName, 
                           CorLoadFlags flags, 
                           DWORD dwFileSize)
{
    PBYTE pFile;
    PBYTE hAddress;
    if(flags == CorLoadOSMap || flags == CorLoadOSImage) {
        hAddress = new (nothrow) BYTE[cbInfo+1];
        if (!hAddress)
            return NULL;

        ZeroMemory(hAddress, cbInfo);
    }
    else
        hAddress = hMemory;

    pFile = hAddress + cbInfo;

    // Copy in the name and include the null terminator
    if(dwFileName) {
        WCHAR* p2 = (WCHAR*) hAddress;
        WCHAR* p1 = (WCHAR*) pFileName;
        while(*p1) {
#if FEATURE_CASE_SENSITIVE_FILESYSTEM
	    *p2++ = *p1++;
#else   // FEATURE_CASE_SENSITIVE_FILESYSTEM
            *p2++ = towlower(*p1++);
#endif  // FEATURE_CASE_SENSITIVE_FILESYSTEM
        }
        *p2 = '\0';
    }
    else
        *((char*)hAddress) = '\0';
    
    // Set the pointer to the file name
    CorMapInfo* ptr = GetMapInfo((HCORMODULE) pFile);
    ptr->pFileName = (LPWSTR) hAddress;
    ptr->SetCorLoadFlags(flags);
    ptr->dwRawSize = dwFileSize;

    if(flags == CorLoadOSMap || flags == CorLoadOSImage)
        ptr->hOSHandle = (HMODULE) hMemory;

    return pFile;
}

BOOL CorMap::MapImageAsData(HANDLE hFile, CorLoadFlags flags, PBYTE *hMapAddress, LPCWSTR pFileName, DWORD dwFileName)
{

    PBYTE hAddress = NULL;
    BOOL fResult = FALSE;
    HANDLE hMapFile = NULL;
    DWORD dwAccessMode = 0;

    // String size + null terminator + a pointer to the string
    DWORD cbInfo = CalculateCorMapInfoSize(dwFileName);
    DWORD dwFileSize = SafeGetFileSize(hFile, 0);
    if (dwFileSize == 0xffffffff)
        return FALSE;

    if(flags == CorLoadOSMap) {

        // Map the file in with copy-on-write pages
        hMapFile = CreateFileMappingA(hFile, NULL, PAGE_WRITECOPY, 0, 0, NULL);
        if(hMapFile == NULL)
            return FALSE;
        dwAccessMode = FILE_MAP_COPY;
    }
    else if(flags == CorLoadDataMap) {
        IMAGE_DOS_HEADER dosHeader;
        IMAGE_NT_HEADERS ntHeader;
        DWORD cbRead = 0;
        DWORD cb = 0;
        if(ReadFile(hFile, &dosHeader, sizeof(dosHeader), &cbRead, NULL) &&
           (cbRead == sizeof(dosHeader)) &&
           (dosHeader.e_magic == VAL16(IMAGE_DOS_SIGNATURE)) &&
           (dosHeader.e_lfanew != 0) &&
           (SetFilePointer(hFile, VAL32(dosHeader.e_lfanew), NULL, FILE_BEGIN) != 0xffffffff) &&
           (ReadFile(hFile, &ntHeader, sizeof(ntHeader), &cbRead, NULL)) &&
           (cbRead == sizeof(ntHeader)) &&
           (ntHeader.Signature == VAL32(IMAGE_NT_SIGNATURE)) &&
           (ntHeader.FileHeader.SizeOfOptionalHeader == VAL16(IMAGE_SIZEOF_NT_OPTIONAL_HEADER)) &&
           (ntHeader.OptionalHeader.Magic == VAL16(IMAGE_NT_OPTIONAL_HDR_MAGIC)))
        {
            cb = VAL32(ntHeader.OptionalHeader.SizeOfImage) + cbInfo;

            // create our swap space in the system swap file
            hMapFile = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, cb, NULL);
            
            if (!hMapFile)
                return FALSE;

            // Align cbInfo to page size so we don't screw up alignment of regular part of image
            cbInfo = (cbInfo + (OS_PAGE_SIZE-1)) & ~(OS_PAGE_SIZE-1);
            dwAccessMode = FILE_MAP_WRITE;
        }
    }


    {
        //That didn't work; maybe the preferred address was taken. Try to
        //map it at any address.
        hAddress = (PBYTE) MapViewOfFile(hMapFile, dwAccessMode, 0, 0, 0);
    }
    
    if (!hAddress)
        goto exit;
    
    // Move the pointer up to the place we will be loading
    hAddress = SetImageName(hAddress, 
                            cbInfo, 
                            pFileName, dwFileName, 
                            flags, 
                            dwFileSize);
    if (hAddress) {
        if (flags == CorLoadDataMap) {
            DWORD cbRead = 0;
            // When we map to an arbitrary location we need to read in the contents
            if((SetFilePointer(hFile, 0, NULL, FILE_BEGIN) == 0xffffffff) ||
               (!ReadFile(hFile, hAddress, dwFileSize, &cbRead, NULL)) ||
               (cbRead != dwFileSize))
                goto exit;
        }

        if(hMapAddress)
            *hMapAddress = hAddress;
    
        fResult = TRUE;
    }
    else
        SetLastError(ERROR_OUTOFMEMORY);

 exit:
    if (hMapFile)
        CloseHandle(hMapFile);
    return fResult;
}

HRESULT CorMap::MemoryMapImage(HCORMODULE hAddress, HCORMODULE* pResult)
{
    HRESULT hr = S_OK;
    BEGIN_ENSURE_PREEMPTIVE_GC();
    Enter();
    END_ENSURE_PREEMPTIVE_GC();
    *pResult = NULL;
    CorMapInfo* ptr = GetMapInfo(hAddress);
    DWORD refs = ptr->References();

    LOG((LF_CLASSLOADER, LL_INFO10, "Remapping file: \"%ws\", %0x, %d (references)\n", ptr->pFileName, ptr, ptr->References()));

    if(refs == (DWORD) -1)
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_ACCESS);
    else if(refs > 1) {
        // If we have more then one ref we need to create a new one.
        CorMapInfo* pCurrent;
        hr = FindFileName(ptr->pFileName, &pCurrent);
        if(FAILED(hr)) {
            // if we are no longer in the table then we will assume we where the one in the table;
            pCurrent = ptr;
            hr = S_OK;
        }

        switch(ptr->Flags()) {
        case CorLoadOSMap:
        case CorLoadDataMap:
            if(pCurrent == ptr) {
                LOG((LF_CLASSLOADER, LL_INFO10, "I am the current mapping: \"%ws\", %0x, %d (references)\n", ptr->pFileName, ptr, ptr->References()));
                // Manually remove the entry and replace it with a new entry. Mark the 
                // old entry as KeepInTable so it does not remove itself from the
                // name hash table during deletion.
                RemoveMapHandle(ptr); // This never returns an error.

                DWORD length = GetFileNameLength(ptr);
                hr = OpenFileInternal(ptr->pFileName, length, 
                                      (ptr->Flags() == CorLoadDataMap) ? CorLoadImageMap : CorLoadOSImage,
                                      pResult);
                if (SUCCEEDED(hr))
                    ptr->SetKeepInTable();
                else {
                    // Other threads may be trying to load this same file -
                    // put it back in the table on failure
                    _ASSERTE(ptr->References() > 1);
                    EEStringData str(length, ptr->pFileName);
                    m_pOpenFiles->InsertValue(&str, (HashDatum) ptr, FALSE);
                }
            }
            else {
                LOG((LF_CLASSLOADER, LL_INFO10, "I am NOT the current mapping: \"%ws\", %0x, %d (references)\n", ptr->pFileName, ptr, ptr->References()));
                ReleaseHandle(hAddress);
                *pResult = GetCorModule(pCurrent);
            }
            break;

        case CorReLoadOSMap:  // For Win9X
            *pResult = hAddress;
            break;

        default:
            hr = HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
        }

    }
    else {
        // if we only have one ref then we should never be reloading it
        _ASSERTE(ptr->Flags() != CorReLoadOSMap);

        // Check to see if we are the only reference or the last of a 
        // series of references. If we are still in the list then we
        // are the only reference
        //
        // This needs to be changed to be the same as when there are multiple references (see above)
        if (*ptr->pFileName == 0) { // byte array images have no name 
            if(SUCCEEDED(hr = LayoutImage((PBYTE) hAddress, (PBYTE) hAddress))) {
                *pResult = hAddress;
            }
        }
        else {
            CorMapInfo* pCurrent;
            IfFailGo(FindFileName(ptr->pFileName, &pCurrent));
            if(pCurrent == ptr) {
                if(SUCCEEDED(hr = LayoutImage((PBYTE) hAddress, (PBYTE) hAddress))) {
                    _ASSERTE(ptr->References() == 2);
                    ptr->Release();
                    *pResult = hAddress;
                }
            }
            else {
                ReleaseHandle(hAddress);
                *pResult = GetCorModule(pCurrent);
            }
        }
    }
 ErrExit:
    Leave();
    return hr;
}

HRESULT CorMap::LayoutImage(PBYTE hAddress,    // Destination of mapping
                            PBYTE pSource)     // Source, different the pSource for InMemory images
{
    HRESULT hr = S_OK;
    CorMapInfo* ptr = GetMapInfo((HCORMODULE) hAddress);
    CorLoadFlags flags = ptr->Flags();

    if(flags == CorLoadOSMap) {
        // If we have a filename, then we'll reload (into R/W memory)
        // but if there's no file name then it's in memory already and
        // we can just use the handle we currently have
        if (ptr->pFileName != NULL && ptr->pFileName[0] != '\0') 
        {
            // Release the OS resources except for the CorMapInfo memory
            /*hr = */ReleaseHandleResources(ptr, FALSE);

            // We lazily load the OS handle
            ptr->hOSHandle = NULL;
        }
        ptr->SetCorLoadFlags(CorLoadOSImage);
        hr = S_FALSE;
    }
    else if(flags == CorLoadDataMap) {
            
        // If we've done our job right, hAddress should be page aligned.
        _ASSERTE(((SIZE_T)hAddress & (OS_PAGE_SIZE-1)) == 0);

        // We can only layout images data images and only ones that
        // are not backed by the file.

        IMAGE_DOS_HEADER* dosHeader;
        IMAGE_NT_HEADERS* ntHeader;
        IMAGE_COR20_HEADER* pCor;
        IMAGE_SECTION_HEADER* rgsh;
        
        IfFailRet(ReadHeaders((PBYTE) pSource, &dosHeader, &ntHeader, &pCor, TRUE));

        rgsh = (IMAGE_SECTION_HEADER*) (pSource + VAL32(dosHeader->e_lfanew) + sizeof(IMAGE_NT_HEADERS));

        if((PBYTE) hAddress != pSource) {
            DWORD cb = VAL32(dosHeader->e_lfanew) + sizeof(IMAGE_NT_HEADERS) +
                sizeof(IMAGE_SECTION_HEADER)*VAL16(ntHeader->FileHeader.NumberOfSections);
            memcpy((void*) hAddress, pSource, cb);

            // Write protect the headers
            DWORD oldProtection;
            if (!VirtualProtect((void *) hAddress, cb, PAGE_READONLY, &oldProtection))
                return HRESULT_FROM_WIN32(GetLastError());
        }

        // now let's loop for each loadable sections
        for (int i = VAL16(ntHeader->FileHeader.NumberOfSections) - 1; i >= 0; i--)
        {
            size_t loff, cbVirt, cbPhys;
            size_t dwAddr;
            
            loff   = VAL32(rgsh[i].PointerToRawData) + (size_t) pSource;
            cbVirt = VAL32(rgsh[i].Misc.VirtualSize);
            cbPhys = min(VAL32(rgsh[i].SizeOfRawData), cbVirt);
            dwAddr = (size_t) VAL32(rgsh[i].VirtualAddress) + (size_t) hAddress;
            
            if(dwAddr >= loff && dwAddr <= loff + cbPhys) {
                // Overlapped copy
                MoveMemory((void*) dwAddr, (void*) loff, cbPhys);
            }
            else {
                // Separate copy
                memcpy((void*) dwAddr, (void*) loff, cbPhys);
            }

            if ((rgsh[i].Characteristics & VAL32(IMAGE_SCN_MEM_WRITE)) == 0) {
                // Write protect the section
                DWORD oldProtection;
                if (!VirtualProtect((void *) dwAddr, cbVirt, PAGE_READONLY, &oldProtection))
                    return HRESULT_FROM_WIN32(GetLastError());
            }
        }

        // Now manually apply base relocs if necessary
        hr = ApplyBaseRelocs(hAddress, ntHeader, ptr);

        ptr->SetCorLoadFlags(CorLoadImageMap);
    }
    return hr;
}

HMODULE CorMap::LoadManagedLibrary(LPWSTR pFileName)
{

    HANDLE hFile = INVALID_HANDLE_VALUE;
    HANDLE hMapFile = INVALID_HANDLE_VALUE;
    BYTE * pModule = NULL;

    LOG((LF_CLASSLOADER, LL_INFO10, "LoadManagedLibrary: \"%ws\"", pFileName));
    
    hFile = WszCreateFile((LPCWSTR) pFileName,
                            GENERIC_READ,
                            FILE_SHARE_READ,
                            NULL,
                            OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                            NULL);

    if (hFile == INVALID_HANDLE_VALUE) 
    {
        goto ErrExit;
    }

    hMapFile = CreateFileMapping(hFile, NULL, PAGE_WRITECOPY, 0, 0, NULL);
    if (hMapFile == NULL)
    {
        goto ErrExit;
    }

    pModule = (PBYTE) MapViewOfFile(hMapFile, FILE_MAP_COPY, 0, 0, 0);

 ErrExit:

    if(hMapFile != INVALID_HANDLE_VALUE)
        CloseHandle(hMapFile);
    if(hFile != INVALID_HANDLE_VALUE)
        CloseHandle(hFile);

    return pModule;
}

BOOL CorMap::LoadMemoryImageW9x(
    PBYTE pUnmappedPE,        // Pointer to the raw file
    DWORD dwUnmappedPE,       // Size of the raw file
    LPCWSTR pImageName,       // [Optional] Image name
    DWORD dwImageName,        // [Optional] Length of Image Name
    HCORMODULE* hMapAddress)  // [out] Created HCORMODULE
{
    IMAGE_DOS_HEADER* dosHeader;
    IMAGE_NT_HEADERS* ntHeader;

    PBYTE hAddress;
    UINT_PTR pOffset;
    HANDLE hMapFile = NULL;

    // String size + null terminator + a pointer to the string
    DWORD cbInfo = CalculateCorMapInfoSize(dwImageName);
    // Align cbInfo to page size so we don't screw up alignment of regular part of image
    cbInfo = (cbInfo + (OS_PAGE_SIZE-1)) & ~(OS_PAGE_SIZE-1);

    dosHeader = (IMAGE_DOS_HEADER*) pUnmappedPE;
    
    if(pUnmappedPE &&
       dwUnmappedPE > sizeof(IMAGE_DOS_HEADER) &&
       (dosHeader->e_magic == VAL16(IMAGE_DOS_SIGNATURE))) {
        
        pOffset = (UINT_PTR) VAL32(dosHeader->e_lfanew) + (UINT_PTR) pUnmappedPE;

        ntHeader = (IMAGE_NT_HEADERS*) pOffset;
        if((dwUnmappedPE >= (DWORD) (pOffset - (UINT_PTR) dosHeader) + sizeof(IMAGE_NT_HEADERS)) &&
           (ntHeader->Signature == VAL32(IMAGE_NT_SIGNATURE)) &&
           (ntHeader->FileHeader.SizeOfOptionalHeader == VAL16(IMAGE_SIZEOF_NT_OPTIONAL_HEADER)) &&
           (ntHeader->OptionalHeader.Magic == VAL16(IMAGE_NT_OPTIONAL_HDR_MAGIC))) 
        {
            DWORD destSize = VAL32(ntHeader->OptionalHeader.SizeOfImage);

            // We can't trust SizeOfImage yet, so make sure we allocate at least enough space
            // for the raw image
            if (dwUnmappedPE > destSize)
                destSize = dwUnmappedPE;

            DWORD cb = destSize + cbInfo;
                
                
            // create our swap space in the system swap file
            hMapFile = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, cb, NULL);
            if (!hMapFile)
                return FALSE;
                
            {
                //That didn't work; maybe the preferred address was taken. Try to
                //map it at any address.
                hAddress = (PBYTE) MapViewOfFile(hMapFile, FILE_MAP_WRITE, 0, 0, cb);
            }
                
            if (!hAddress)
                goto exit;
                
            hAddress = SetImageName(hAddress, 
                                    cbInfo,
                                    pImageName, dwImageName,
                                    CorLoadDataMap, dwUnmappedPE);
            if (!hAddress)
                SetLastError(ERROR_OUTOFMEMORY);
            else {
                memcpyNoGCRefs(hAddress, pUnmappedPE, dwUnmappedPE);
                if(hMapAddress)
                    *hMapAddress = (HCORMODULE) hAddress;
                CloseHandle(hMapFile);
                return TRUE;
            }
        }
    }

 exit:
    if (hMapFile)
        CloseHandle(hMapFile);
    return FALSE;

}

HRESULT CorMap::OpenFile(LPCWSTR szPath, CorLoadFlags flags, HCORMODULE *ppHandle, DWORD *pdwLength)
{
    if(szPath == NULL || ppHandle == NULL)
        return E_POINTER;

    HRESULT hr;
    WCHAR pPath[_MAX_PATH];
    DWORD size = sizeof(pPath) / sizeof(WCHAR);

    hr = BuildFileName(szPath, (LPWSTR) pPath, &size);
    IfFailRet(hr);

    BEGIN_ENSURE_PREEMPTIVE_GC();
    Enter();
    END_ENSURE_PREEMPTIVE_GC();

    hr = OpenFileInternal(pPath, size, flags, ppHandle, pdwLength);
    Leave();
    return hr;
}

HRESULT CorMap::OpenFileInternal(LPCWSTR pPath, DWORD size, CorLoadFlags flags, HCORMODULE *ppHandle, DWORD *pdwLength)
{
    HRESULT hr;
    CorMapInfo* pResult = NULL;
    HANDLE hFile = INVALID_HANDLE_VALUE;

    Thread *thread = GetThread();
    BOOL    toggleGC = (thread && thread->PreemptiveGCDisabled());

    LOG((LF_CLASSLOADER, LL_INFO10, "Open Internal: \"%ws\", flags = %d\n", pPath, flags));
    
    if (toggleGC)
        thread->EnablePreemptiveGC();

    IfFailGo(InitializeTable());

    if(flags == CorReLoadOSMap) {
        hr = E_FAIL;
        flags = CorLoadImageMap;
    }
    else {
        hr = FindFileName((LPCWSTR) pPath, &pResult);
#ifdef _DEBUG
        if(pResult == NULL)
            LOG((LF_CLASSLOADER, LL_INFO10, "Did not find a preloaded info, hr = %x\n", hr));
        else
            LOG((LF_CLASSLOADER, LL_INFO10, "Found mapinfo: \"%ws\", hr = %x\n", pResult->pFileName,hr));
#endif
    }

    if(FAILED(hr) || pResult == NULL) {
        hFile = WszCreateFile((LPCWSTR) pPath,
                              GENERIC_READ,
                              FILE_SHARE_READ,
                              NULL,
                              OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                              NULL);
        if (hFile == INVALID_HANDLE_VALUE) 
            hr = HRESULT_FROM_WIN32(GetLastError());
        else {
            // Start up the image, this will store off the handle so we do not have 
            // to close it unless there was an error.
            HCORMODULE mappedData;
            hr = LoadImage(hFile, flags, (PBYTE*) &mappedData, (LPCWSTR) pPath, size);

            if(SUCCEEDED(hr)) {
                pResult = GetMapInfo(mappedData);
                hr = AddFile(pResult);
                //_ASSERTE(hr != S_FALSE);
            }
        }
    }
    else {
        CorLoadFlags image = pResult->Flags();
        if(image == CorLoadImageMap || image == CorLoadOSImage)
        {
            hr = S_FALSE;
        }
    }

    if(SUCCEEDED(hr) && pResult) {
        *ppHandle = GetCorModule(pResult);
        if (pdwLength)
            *pdwLength = pResult->dwRawSize;
    }

 ErrExit:

    if(hFile != INVALID_HANDLE_VALUE)
        CloseHandle(hFile);
    if (toggleGC)
        thread->DisablePreemptiveGC();

    return hr;
}

HRESULT CorMap::VerifyDirectory(IMAGE_NT_HEADERS* pNT, IMAGE_DATA_DIRECTORY *dir, DWORD dwForbiddenCharacteristics) 
{
    // Under CE, we have no NT header.
    if (pNT == NULL)
        return S_OK;

    int section_num = 1;
    int max_section = VAL16(pNT->FileHeader.NumberOfSections);

    IMAGE_SECTION_HEADER* pCurrSection = IMAGE_FIRST_SECTION(pNT);
    IMAGE_SECTION_HEADER* prevSection = NULL;

    if (dir->VirtualAddress == NULL && dir->Size == NULL)
        return S_OK;

    // find which section the (input) RVA belongs to
    while (VAL32(dir->VirtualAddress) >= VAL32(pCurrSection->VirtualAddress) 
           && section_num <= max_section)
    {
        section_num++;
        prevSection = pCurrSection;
        pCurrSection++;
    }

    // check if (input) size fits within section size
    if (prevSection != NULL)     
    {
        if (VAL32(dir->VirtualAddress) <= VAL32(prevSection->VirtualAddress) + VAL32(prevSection->Misc.VirtualSize))
        {
            if ((VAL32(dir->VirtualAddress) + VAL32(dir->Size) 
                <= VAL32(prevSection->VirtualAddress) + VAL32(prevSection->Misc.VirtualSize))
                && ((VAL32(prevSection->Characteristics) & dwForbiddenCharacteristics) == 0))
                return S_OK;
        }
    }   

    return HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
}

HRESULT CorMap::ReadHeaders(PBYTE hAddress, IMAGE_DOS_HEADER** ppDos, IMAGE_NT_HEADERS** ppNT, IMAGE_COR20_HEADER** ppCor, BOOL fDataMap, DWORD dwLength)
{
    _ASSERTE(ppNT);
    _ASSERTE(ppCor);

    IMAGE_DOS_HEADER* pDOS = (IMAGE_DOS_HEADER*) hAddress;
    IMAGE_NT_HEADERS* pNT;
    IMAGE_DATA_DIRECTORY *entry = NULL;

    if ((pDOS->e_magic != VAL16(IMAGE_DOS_SIGNATURE)) ||
        (pDOS->e_lfanew == 0))
        return HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);

    *ppDos = pDOS;

    pNT = (IMAGE_NT_HEADERS*) (VAL32(pDOS->e_lfanew) + hAddress);

    //is this a 64 or 32bit image?
	if (pNT->OptionalHeader.Magic == VAL16(IMAGE_NT_OPTIONAL_HDR32_MAGIC))
	{

	    IMAGE_NT_HEADERS32 *pNT32 = (IMAGE_NT_HEADERS32*)pNT;

		if ((dwLength != 0)  && 
			(dwLength != (DWORD) -1) &&
			((UINT) (VAL32(pDOS->e_lfanew) + sizeof(IMAGE_NT_HEADERS32)) > (UINT) dwLength)) 
		{
			_ASSERTE(!"CorMap::ReadHeaders -- bad IMAGE_NT_HEADERS pointer");
			return HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
		}

		if ((pNT32->Signature != VAL32(IMAGE_NT_SIGNATURE)) ||
			(pNT32->FileHeader.SizeOfOptionalHeader != VAL16(IMAGE_SIZEOF_NT_OPTIONAL32_HEADER)) ||
			(pNT32->OptionalHeader.Magic != VAL16(IMAGE_NT_OPTIONAL_HDR32_MAGIC)))
			return HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);

		entry = &pNT32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COMHEADER];
	    
		if (VAL32(entry->VirtualAddress) == 0 || VAL32(entry->Size) == 0
			|| VAL32(entry->Size) < sizeof(IMAGE_COR20_HEADER))
			return HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);

	}
	else if(pNT->OptionalHeader.Magic == VAL16(IMAGE_NT_OPTIONAL_HDR64_MAGIC))
	{
	    IMAGE_NT_HEADERS64 *pNT64 = (IMAGE_NT_HEADERS64*)pNT;

		if ((dwLength != 0)  && 
			(dwLength != (DWORD) -1) &&
			((UINT) (VAL32(pDOS->e_lfanew) + sizeof(IMAGE_NT_HEADERS64)) > (UINT) dwLength)) 
		{
			_ASSERTE(!"CorMap::ReadHeaders -- bad IMAGE_NT_HEADERS pointer");
			return HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
		}

		if ((pNT64->Signature != VAL32(IMAGE_NT_SIGNATURE)) ||
			(pNT64->FileHeader.SizeOfOptionalHeader != VAL16(IMAGE_SIZEOF_NT_OPTIONAL64_HEADER)) ||
			(pNT64->OptionalHeader.Magic != VAL16(IMAGE_NT_OPTIONAL_HDR64_MAGIC)))
			return HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);

		entry = &pNT64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COMHEADER];
	    
		if (VAL32(entry->VirtualAddress) == 0 || VAL32(entry->Size) == 0
			|| VAL32(entry->Size) < sizeof(IMAGE_COR20_HEADER))
			return HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
	}
	else
	{
		return HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
	}
	
    *ppNT = pNT;    // this needs to be set before we call VerifyDirectory

    //verify RVA and size of the COM+ header
    HRESULT hr = VerifyDirectory(pNT, entry,IMAGE_SCN_MEM_WRITE);
    if (FAILED(hr))
        return hr;

    DWORD offset;
    if(fDataMap)
        offset = Cor_RtlImageRvaToOffset(pNT, VAL32(entry->VirtualAddress), dwLength);
    else 
        offset = VAL32(entry->VirtualAddress);

    *ppCor = (IMAGE_COR20_HEADER *) (offset + hAddress);
    return S_OK;
}

//-----------------------------------------------------------------------------
// openFile - maps pszFileName to memory, returns ptr to it (read-only)
// (Approximates LoadLibrary for Win9X)
// Caller must call FreeLibrary((HINSTANCE) *hMapAddress) and may need to delete[] szFilePath when finished.
//-----------------------------------------------------------------------------
HRESULT CorMap::OpenRawImage(PBYTE pUnmappedPE, DWORD dwUnmappedPE, LPCWSTR pwszFileName, HCORMODULE *hHandle, BOOL fResource/*=FALSE*/)
{
    HRESULT hr = S_OK;
    _ASSERTE(hHandle);
    if(hHandle == NULL) return E_POINTER;

    Thread *thread = GetThread();
    BOOL    toggleGC = (thread && thread->PreemptiveGCDisabled());

    if (toggleGC)
        thread->EnablePreemptiveGC();

    int length = 0;
    if(pwszFileName) length = (int)wcslen(pwszFileName);
    hr = LoadFile(pUnmappedPE, dwUnmappedPE, pwszFileName, length, fResource, hHandle);

    if (toggleGC)
        thread->DisablePreemptiveGC();

    return hr;
}

HRESULT CorMap::LoadFile(PBYTE pUnmappedPE, DWORD dwUnmappedPE, LPCWSTR pImageName,
                         DWORD dwImageName, BOOL fResource, HCORMODULE *phHandle)
{

    HRESULT hr = S_OK;

    CorMapInfo* pResult = NULL;
    WCHAR pPath[_MAX_PATH];
    DWORD size = sizeof(pPath) / sizeof(WCHAR);

    IfFailRet(InitializeTable());

    if(pImageName) {
        IfFailRet(BuildFileName(pImageName, (LPWSTR) pPath, &size));
        pImageName = pPath;
        dwImageName = size;
    }


    BEGIN_ENSURE_PREEMPTIVE_GC();
    Enter();
    END_ENSURE_PREEMPTIVE_GC();
    if(pImageName) 
        hr = FindFileName(pImageName, &pResult);

    if(FAILED(hr) || pResult == NULL) {
        HCORMODULE mappedData;

        BOOL fResult = TRUE;
        if (fResource)
            fResult = LoadMemoryResource(pUnmappedPE, dwUnmappedPE, pImageName, dwImageName, &mappedData);
        else 
        {
            fResult = LoadMemoryImageW9x(pUnmappedPE, dwUnmappedPE, pImageName, dwImageName, &mappedData);
        }

        if (!fResult) {
            hr = HRESULT_FROM_WIN32(GetLastError());
            if (SUCCEEDED(hr))
                hr = HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
        }
        else {
            pResult = GetMapInfo(mappedData);
            if(pImageName) 
                hr = AddFile(pResult);
            else
            {
                pResult->AddRef();
                hr = S_OK;
            }
        }
    }
    Leave();

    if(SUCCEEDED(hr))
        *phHandle = GetCorModule(pResult);
        
    return hr;
}

// For loading a file that is a CLR resource, not a PE file.
BOOL CorMap::LoadMemoryResource(PBYTE pbResource, DWORD dwResource, 
                                LPCWSTR pImageName, DWORD dwImageName, HCORMODULE* hMapAddress)
{
    if (!pbResource)
        return FALSE;

    DWORD cbNameSize = CalculateCorMapInfoSize(dwImageName);
    DWORD cb = dwResource + cbNameSize;

    HANDLE hMapFile = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, cb, NULL);
    if (!hMapFile)
        return FALSE;
                    
    PBYTE hAddress = (PBYTE) MapViewOfFile(hMapFile, FILE_MAP_WRITE, 0, 0, cb);
    if (!hAddress) {
        CloseHandle(hMapFile);
        return FALSE;
    }

    // Move the pointer up to the place we will be loading
    hAddress = SetImageName(hAddress,
                            cbNameSize,
                            pImageName,
                            dwImageName,
                            CorLoadDataMap,
                            dwResource);
    if (!hAddress) {
        SetLastError(ERROR_OUTOFMEMORY);
        CloseHandle(hMapFile);
        return FALSE;
    }

    // Copy the data
    memcpy((void*) hAddress, pbResource, dwResource);
    *hMapAddress = (HCORMODULE) hAddress;

    CloseHandle(hMapFile);
    return TRUE;
}

HRESULT CorMap::ApplyBaseRelocs(PBYTE hAddress, IMAGE_NT_HEADERS *pNT, CorMapInfo* ptr)
{
    if(ptr->RelocsApplied() == TRUE)
        return S_OK;


    SIZE_T delta = (SIZE_T) (hAddress - VAL32(pNT->OptionalHeader.ImageBase));
    if (delta == 0)
        return S_FALSE;

    if (GetAppDomain()->IsCompilationDomain())
    {
        // For a compilation domain, we treat the image specially.  Basically we don't
        // care about base relocs, except for one special case - the TLS directory. 
        // We can find this one by hand, and we need to work in situations even where
        // base relocs have been stripped.  So basically no matter what the base relocs
        // say we just fix up this one section.

        IMAGE_DATA_DIRECTORY *tls = &pNT->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS];
        if (tls->VirtualAddress != 0 && tls->Size > 0)
        {
            IMAGE_TLS_DIRECTORY *pDir = (IMAGE_TLS_DIRECTORY*) (VAL32(tls->VirtualAddress) + hAddress);
            if (pDir != NULL)
            {
                DWORD oldProtection;
                if (VirtualProtect((VOID *) pDir, sizeof(*pDir), PAGE_READWRITE,
                                   &oldProtection))
                {
                    pDir->StartAddressOfRawData = VAL32(VAL32(pDir->StartAddressOfRawData) + delta);
                    pDir->EndAddressOfRawData = VAL32(VAL32(pDir->EndAddressOfRawData) + delta);
                    pDir->AddressOfIndex = VAL32(VAL32(pDir->AddressOfIndex) + delta);
                    pDir->AddressOfCallBacks = VAL32(VAL32(pDir->AddressOfCallBacks) + delta);

                    VirtualProtect((VOID *) pDir, sizeof(*pDir), oldProtection,
                                   &oldProtection);
                }
            }
        }

        ptr->SetRelocsApplied();
        return S_FALSE;
    }
    
    // 
    // If base relocs have been stripped, we must fail.
    // 

    if (pNT->FileHeader.Characteristics & VAL16(IMAGE_FILE_RELOCS_STRIPPED)) {
        ptr->SetRelocsApplied();
        STRESS_ASSERT(0);
        BAD_FORMAT_ASSERT(!"Relocs stripped");
        return COR_E_BADIMAGEFORMAT;
    }

    //
    // Look for a base relocs section.
    //

    IMAGE_DATA_DIRECTORY *dir 
      = &pNT->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];

    if (dir->VirtualAddress != 0)
    {
        BYTE *relocations = hAddress + VAL32(dir->VirtualAddress);
        BYTE *relocationsEnd = relocations + VAL32(dir->Size);

        while (relocations < relocationsEnd)
        {
            IMAGE_BASE_RELOCATION *r = (IMAGE_BASE_RELOCATION*) relocations;

            SIZE_T pageAddress = (SIZE_T) (hAddress + VAL32(r->VirtualAddress));

            DWORD oldProtection;
            
            // The +1 on PAGE_SIZE forces the subsequent page to change protection rights
            // as well - this fixes certain cases where the final write occurs across a page boundary.
            if (VirtualProtect((VOID *) pageAddress, PAGE_SIZE+1, PAGE_READWRITE,
                               &oldProtection))
            {
                USHORT *fixups = (USHORT *) (r+1);
                USHORT *fixupsEnd = (USHORT *) ((BYTE*)r + VAL32(r->SizeOfBlock));

                while (fixups < fixupsEnd)
                {
                    USHORT fixup = VAL16(*fixups);
 
                    if ((fixup>>12) != IMAGE_REL_BASED_ABSOLUTE)
                    {
                        // Only support HIGHLOW fixups for now
                        _ASSERTE((fixup>>12) == IMAGE_REL_BASED_HIGHLOW);

                        if ((fixup>>12) != IMAGE_REL_BASED_HIGHLOW) {
                            ptr->SetRelocsApplied();
                            STRESS_ASSERT(0);
                            BAD_FORMAT_ASSERT(!"Bad Reloc");
                            return COR_E_BADIMAGEFORMAT;
                        }

                        SIZE_T *paddress = (SIZE_T*)(pageAddress + (fixup&0xfff));
                        SIZE_T address = VAL32(*paddress);

                        if ((address < VAL32(pNT->OptionalHeader.ImageBase)) ||
                            (address >= (VAL32(pNT->OptionalHeader.ImageBase)
                                          + VAL32(pNT->OptionalHeader.SizeOfImage)))) {
                            ptr->SetRelocsApplied();
                            STRESS_ASSERT(0);
                            BAD_FORMAT_ASSERT(!"Bad Reloc");
                            return COR_E_BADIMAGEFORMAT;
                        }

                        address += delta;
                        *paddress = VAL32(address);

                        if ((address < (SIZE_T) hAddress) ||
                            (address >= ((SIZE_T) hAddress 
                                          + VAL32(pNT->OptionalHeader.SizeOfImage)))) {
                            ptr->SetRelocsApplied();
                            STRESS_ASSERT(0);
                            BAD_FORMAT_ASSERT(!"Bad Reloc");
                            return COR_E_BADIMAGEFORMAT;
                        }
                    }

                    fixups++;
                }

                VirtualProtect((VOID *) pageAddress, PAGE_SIZE+1, oldProtection,
                               &oldProtection);
            }

            relocations += (VAL32(r->SizeOfBlock)+3)&~3; // pad to 4 bytes
        }
    }

    // If we find an IAT with more than 2 entries (which we expect from our loader
    // thunk), fail the load.

    if (VAL32(pNT->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT].VirtualAddress) != 0
         && (VAL32(pNT->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT].Size) > 
             (2*sizeof(IMAGE_THUNK_DATA)))) {
        ptr->SetRelocsApplied();
        return COR_E_FIXUPSINEXE;
    }

    // If we have a TLS section, fail the load.

    if (VAL32(pNT->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress) != 0
        && VAL32(pNT->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].Size) > 0) {
        ptr->SetRelocsApplied();
        return COR_E_FIXUPSINEXE;
    }
    

    ptr->SetRelocsApplied();
    return S_OK;
}

//-----------------------------------------------------------------------------
// mapFile - maps an hFile to memory, returns ptr to it (read-only)
// Caller must call FreeLibrary((HINSTANCE) *hMapAddress) when finished.
//-----------------------------------------------------------------------------
HRESULT CorMap::MapFile(HANDLE hFile, HMODULE *phHandle)
{
    HRESULT hr;
    PBYTE pMapAddress = NULL;

    _ASSERTE(phHandle);
    if(phHandle == NULL) return E_POINTER;
    BEGIN_ENSURE_PREEMPTIVE_GC();
    Enter();
    END_ENSURE_PREEMPTIVE_GC();
    hr = LoadImage(hFile, CorLoadImageMap, &pMapAddress, NULL, 0);
    Leave();
    if(SUCCEEDED(hr))
        *phHandle = (HMODULE) pMapAddress;
    return S_OK;
}

HRESULT CorMap::InitializeTable()
{
   HRESULT hr = S_OK;
   if(m_pOpenFiles == NULL) {
       BEGIN_ENSURE_PREEMPTIVE_GC();
       Enter();
       END_ENSURE_PREEMPTIVE_GC();
       if(m_pOpenFiles == NULL) {
           m_pOpenFiles = new (nothrow) EEUnicodeStringHashTable();
           if(m_pOpenFiles == NULL) 
               hr = E_OUTOFMEMORY;
           else {
               LockOwner lock = {&m_pCorMapCrst, IsOwnerOfOSCrst};
               if(!m_pOpenFiles->Init(BLOCK_SIZE, &lock))
                   hr = E_OUTOFMEMORY;
           }
       }
       Leave();
   }

   return hr;
}

HRESULT CorMap::BuildFileName(LPCWSTR pszFileName, LPWSTR pPath, DWORD* dwPath)
{
    // Strip off any file protocol
    if(_wcsnicmp(pszFileName, L"file://", 7) == 0) {
        pszFileName = pszFileName+7;
        if(*pszFileName == L'/') pszFileName++;
    }
    
    // Get the absolute file name, size does not include the null terminator
    LPWSTR fileName;
    DWORD dwName = WszGetFullPathName(pszFileName, *dwPath, pPath, &fileName);
    if(dwName == 0) {
        return HRESULT_FROM_WIN32(GetLastError());
    }
    else if(dwName > *dwPath) {
        *dwPath = dwName + 1;
        return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
    }
    *dwPath = dwName + 1;
    
#if !FEATURE_CASE_SENSITIVE_FILESYSTEM
    WCHAR* ptr = pPath;
    while(*ptr) {
        *ptr = towlower(*ptr);
        ptr++;
    }
#endif  // !FEATURE_CASE_SENSITIVE_FILESYSTEM
    return S_OK;
}

HRESULT CorMap::FindFileName(LPCWSTR pszFileName, CorMapInfo** pHandle)
{
    _ASSERTE(pHandle);
    _ASSERTE(pszFileName);
    _ASSERTE(m_fInitialized);
    _ASSERTE(m_fInsideMapLock > 0);

    HRESULT hr;
    HashDatum data;
    *pHandle = NULL;

    LOG((LF_CLASSLOADER, LL_INFO10, "FindFileName: \"%ws\", %d\n", pszFileName, wcslen(pszFileName)+1));

    DWORD length = (DWORD) wcslen(pszFileName) + 1;
    EEStringData str(length, pszFileName);
    if(m_pOpenFiles->GetValue(&str, &data)) {
        *pHandle = (CorMapInfo*) data;
        (*pHandle)->AddRef();
        hr = S_OK;
    }
    else
        hr = E_FAIL;

    return hr;
}

        
HRESULT CorMap::AddFile(CorMapInfo* ptr)
{
    HRESULT hr;

    _ASSERTE(m_fInsideMapLock > 0);
    _ASSERTE((ptr->References() & 0xffff0000) == 0);

    LPCWSTR psName = ptr->pFileName;
    DWORD length = GetFileNameLength(psName, (UINT_PTR) ptr);
    
    LOG((LF_CLASSLOADER, LL_INFO10, "Adding file to Open list: \"%ws\", %0x, %d\n", psName, ptr, length));

    EEStringData str(length, psName);
    HashDatum pData;
    if(m_pOpenFiles->GetValue(&str, &pData)) {
        ptr->AddRef();
        return S_FALSE;
    }

    
    pData = (HashDatum) ptr;       
    if(m_pOpenFiles->InsertValue(&str, pData, FALSE)) {
        ptr->AddRef();
        hr = S_OK;
    }
    else
        hr = E_OUTOFMEMORY;
    return hr;
}

HRESULT CorMap::ReleaseHandleResources(CorMapInfo* ptr, BOOL fDeleteHandle)
{
    _ASSERTE(ptr);
    _ASSERTE((ptr->References() & 0xffff0000) == 0);

    CorLoadFlags flags = ptr->Flags();
    HRESULT hr = S_OK;
    HANDLE hModule = NULL;
    HCORMODULE pChild;

    switch(flags) {
    case CorLoadOSImage:
        if(ptr->hOSHandle) {
            if (!g_fProcessDetach)
                FreeLibrary(ptr->hOSHandle);
            ptr->hOSHandle = NULL;
        }
        if(fDeleteHandle) 
            delete (PBYTE) GetMemoryStart(ptr);
        break;
    case CorLoadOSMap:
        if(ptr->hOSHandle) {
            hModule = ptr->hOSHandle;
            ptr->hOSHandle = NULL;
        }
        if(fDeleteHandle)
            delete (PBYTE) GetMemoryStart(ptr);
        break;
    case CorReLoadOSMap:
        pChild = (HCORMODULE) ptr->hOSHandle;
        if(pChild) 
            ReleaseHandle(pChild);
        break;
    case CorLoadDataMap:
    case CorLoadImageMap:
        hModule = (HMODULE) GetMemoryStart(ptr);
    default:
	    break;
    }
    
    if(hModule != NULL && !UnmapViewOfFile(hModule)) // Filename is the beginning of the mapped data
        hr = HRESULT_FROM_WIN32(GetLastError());

    return hr;
}

HRESULT CorMap::RemoveMapHandle(CorMapInfo*  pInfo)
{
    _ASSERTE(pInfo);
    _ASSERTE((pInfo->References() & 0xffff0000) == 0);

    _ASSERTE(m_fInsideMapLock > 0);

    if(m_pOpenFiles == NULL || pInfo->KeepInTable()) return S_OK;
    
    LPCWSTR fileName = pInfo->pFileName;
    DWORD length = GetFileNameLength(fileName, (UINT_PTR) pInfo);

    EEStringData str(length, fileName);
    m_pOpenFiles->DeleteValue(&str);
    
    return S_OK;
}

DWORD CorMap::GetFileNameLength(CorMapInfo* ptr)
{
    _ASSERTE(ptr);
    _ASSERTE((ptr->References() & 0xffff0000) == 0);
    return GetFileNameLength(ptr->pFileName, (UINT_PTR) ptr);
}

DWORD CorMap::GetFileNameLength(LPCWSTR name, UINT_PTR start)
{
    DWORD length;

    length = (DWORD)(start - (UINT_PTR) (name)) / sizeof(WCHAR);

    WCHAR* tail = ((WCHAR*)name) + length - 1;
    while(tail >= name && *tail == L'\0') tail--; // remove all nulls

    if(tail > name)
        length = (DWORD)(tail - name + 2); // the last character and the null character back in
    else
        length = 0;

    return length;
}

void CorMap::GetFileName(HCORMODULE pHandle, WCHAR *psBuffer, DWORD dwBuffer, DWORD *pLength)
{
    DWORD length = 0;
    CorMapInfo* pInfo = GetMapInfo(pHandle);
    _ASSERTE((pInfo->References() & 0xffff0000) == 0);

    LPWSTR name = pInfo->pFileName;

    if (name)
    {
        length = GetFileNameLength(name, (UINT_PTR) pInfo);
        if(dwBuffer > 0) {
            length = length > dwBuffer ? dwBuffer : length;
            memcpy(psBuffer, name, length*sizeof(WCHAR));
        }
    }

    *pLength = length;
}

void CorMap::GetFileName(HCORMODULE pHandle, char* psBuffer, DWORD dwBuffer, DWORD *pLength)
{
    DWORD length = 0;
    CorMapInfo* pInfo = (((CorMapInfo *) pHandle) - 1);
    _ASSERTE((pInfo->References() & 0xffff0000) == 0);

    LPWSTR name = pInfo->pFileName;
    if (*name)
    {
        length = GetFileNameLength(name, (UINT_PTR) pInfo);
        length = WszWideCharToMultiByte(CP_UTF8, 0, name, length, psBuffer, dwBuffer, 0, NULL);
    }

    *pLength = length;
}

CorLoadFlags CorMap::ImageType(HCORMODULE pHandle)
{
    CorMapInfo* ptr = GetMapInfo(pHandle);
    _ASSERTE((ptr->References() & 0xffff0000) == 0);
    return ptr->Flags();
}

HRESULT CorMap::BaseAddress(HCORMODULE pHandle, HMODULE* pModule)
{
    HRESULT hr = S_OK;
    CorMapInfo* ptr = GetMapInfo(pHandle);
    _ASSERTE(pModule);
    _ASSERTE((ptr->References() & 0xffff0000) == 0);
    HMODULE hMod = NULL;

    switch(ptr->Flags()) {
    case CorLoadOSImage:
        if(ptr->hOSHandle == NULL) {
            if(!ptr->pFileName)
                hr = HRESULT_FROM_WIN32(ERROR_INVALID_NAME);
        
            // If LoadLibrary succeeds then the hOSHandle should be set by the
            // ExecuteDLL() entry point
#ifdef _DEBUG           
            if(!g_pConfig->UseBuiltInLoader())
#endif
            {
                hMod = LoadManagedLibrary(ptr->pFileName);
            }

            if(hMod == NULL) {
                {
                    hr = HRESULT_FROM_WIN32(GetLastError());
                    ptr->SetCorLoadFlags(CorLoadUndefinedMap);
                }
            }
            else
            {      
                HMODULE old = (HMODULE) FastInterlockCompareExchangePointer((PVOID*)&(ptr->hOSHandle), hMod, NULL);

                // If there was a previous value then we'll use it and free the new one
                if(old != NULL) {
                    UnmapViewOfFile(hMod);
                }
            }
        }
        *pModule = ptr->hOSHandle;
        break;
    case CorReLoadOSMap:
    case CorLoadOSMap:
       *pModule = ptr->hOSHandle;
       break;
    case CorLoadUndefinedMap:
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_HANDLE);
        break;
    default:
        *pModule = (HMODULE) pHandle;
        break;
    }
    return hr;
}

size_t CorMap::GetRawLength(HCORMODULE pHandle)
{
    CorMapInfo* ptr = GetMapInfo(pHandle);
    _ASSERTE((ptr->References() & 0xffff0000) == 0);
    return ptr->dwRawSize;
}

HRESULT CorMap::AddRefHandle(HCORMODULE pHandle)
{
    _ASSERTE(pHandle);
    CorMapInfo* ptr = GetMapInfo(pHandle);
    _ASSERTE((ptr->References() & 0xffff0000) == 0);
    ptr->AddRef();
    return S_OK;
}

HRESULT CorMap::ReleaseHandle(HCORMODULE pHandle)
{
    _ASSERTE(pHandle);
    CorMapInfo* ptr = GetMapInfo(pHandle);
    _ASSERTE((ptr->References() & 0xffff0000) == 0);
    ptr->Release();
    return S_OK;
}

void CorMap::EnterSpinLock ()
{ 
    while (1)
    {
        if (FastInterlockExchange (&m_spinLock, 1) == 1)
            __SwitchToThread(5);
        else
            return;
    }
}

//---------------------------------------------------------------------------------
//

ULONG CorMapInfo::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

ULONG CorMapInfo::Release()
{
   BEGIN_ENSURE_PREEMPTIVE_GC();
   CorMap::Enter();
   END_ENSURE_PREEMPTIVE_GC();
    
   ULONG   cRef = InterlockedDecrement(&m_cRef);
   if (!cRef) {


       CorMap::RemoveMapHandle(this);
       CorMap::ReleaseHandleResources(this, TRUE);
   }

   BEGIN_ENSURE_PREEMPTIVE_GC();
   CorMap::Leave();
   END_ENSURE_PREEMPTIVE_GC();

   return (cRef);
}
