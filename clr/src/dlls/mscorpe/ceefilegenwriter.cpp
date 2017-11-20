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
// Derived class from CCeeGen which handles writing out
// the exe. All references to PEWriter pulled out of CCeeGen,
// and moved here
//
//

#include "stdafx.h"

#include <string.h>
#include <limits.h>

#include "corerror.h"
#include "stubs.h"
#include <posterror.h>

// Get the Symbol entry given the head and a 0-based index
IMAGE_SYMBOL* GetSymbolEntry(IMAGE_SYMBOL* pHead, int idx)
{
    return (IMAGE_SYMBOL*) (((BYTE*) pHead) + IMAGE_SIZEOF_SYMBOL * idx);
}

//*****************************************************************************
// To get a new instance, call CreateNewInstance() instead of new
//*****************************************************************************

HRESULT CeeFileGenWriter::CreateNewInstance(CCeeGen *pCeeFileGenFrom, CeeFileGenWriter* & pGenWriter)
{   
    pGenWriter = new CeeFileGenWriter;
    TESTANDRETURN(pGenWriter, E_OUTOFMEMORY);
    
    PEWriter *pPEWriter = new PEWriter;
    TESTANDRETURN(pPEWriter, E_OUTOFMEMORY);
    //HACK HACK HACK.  
    //What's really the correct thing to be doing here?
    //HRESULT hr = pPEWriter->Init(pCeeFileGenFrom ? pCeeFileGenFrom->getPESectionMan() : NULL);
    HRESULT hr = pPEWriter->Init(NULL);
    TESTANDRETURNHR(hr);

    //Create the general PEWriter.
    pGenWriter->m_peSectionMan = pPEWriter;
    hr = pGenWriter->Init(); // base class member to finish init
    TESTANDRETURNHR(hr);

    pGenWriter->setImageBase(CEE_IMAGE_BASE); // use same default as linker
    pGenWriter->setSubsystem(IMAGE_SUBSYSTEM_WINDOWS_CUI, CEE_IMAGE_SUBSYSTEM_MAJOR_VERSION, CEE_IMAGE_SUBSYSTEM_MINOR_VERSION);

    hr = pGenWriter->allocateIAT(); // so iat goes out first
    TESTANDRETURNHR(hr);

    hr = pGenWriter->allocateCorHeader();   // so cor header near front
    TESTANDRETURNHR(hr);

    //If we were passed a CCeeGen at the beginning, copy it's data now.
    if (pCeeFileGenFrom) {
        pCeeFileGenFrom->cloneInstance((CCeeGen*)pGenWriter);
    }

    // set il RVA to be after the preallocated sections
    pPEWriter->setIlRva(pGenWriter->m_iDataSectionIAT->dataLen());
    return hr;
} // HRESULT CeeFileGenWriter::CreateNewInstance()

CeeFileGenWriter::CeeFileGenWriter() // ctor is protected
{
    m_outputFileName = NULL;
    m_resourceFileName = NULL;
    m_dllSwitch = false;
    m_objSwitch = false;
    m_libraryName = NULL;
    m_libraryGuid = GUID_NULL;

    m_entryPoint = 0;
    m_comImageFlags = COMIMAGE_FLAGS_ILONLY;    // ceegen PEs don't have native code
    m_iatOffset = 0;

    m_dwMacroDefinitionSize = 0;
    m_dwMacroDefinitionRVA = NULL;

    m_dwManifestSize = 0;
    m_dwManifestRVA = NULL;

    m_dwStrongNameSize = 0;
    m_dwStrongNameRVA = NULL;

    m_dwVTableSize = 0;
    m_dwVTableRVA = NULL;

    m_linked = false;
    m_fixed = false;
} // CeeFileGenWriter::CeeFileGenWriter()

//*****************************************************************************
// Cleanup
//*****************************************************************************
HRESULT CeeFileGenWriter::Cleanup() // virtual 
{
    ((PEWriter *)m_peSectionMan)->Cleanup();  // call derived cleanup
    delete m_peSectionMan;
    m_peSectionMan = NULL; // so base class won't delete

    delete[] m_outputFileName;
    delete[] m_resourceFileName;

    if (m_iDataDlls) {
        for (int i=0; i < m_dllCount; i++) {
            if (m_iDataDlls[i].m_methodName)
                free(m_iDataDlls[i].m_methodName);
        }
        free(m_iDataDlls);
    }

    return CCeeGen::Cleanup();
} // HRESULT CeeFileGenWriter::Cleanup()

HRESULT CeeFileGenWriter::EmitMacroDefinitions(void *pData, DWORD cData)
{

    m_dwMacroDefinitionSize = 0;

    
    return S_OK;
} // HRESULT CeeFileGenWriter::EmitMacroDefinitions()

HRESULT CeeFileGenWriter::link()
{
    HRESULT hr = checkForErrors();
    if (! SUCCEEDED(hr))
        return hr;


    m_corHeader->Resources.VirtualAddress = VAL32(m_dwManifestRVA);
    m_corHeader->Resources.Size = VAL32(m_dwManifestSize);

    m_corHeader->StrongNameSignature.VirtualAddress = VAL32(m_dwStrongNameRVA);
    m_corHeader->StrongNameSignature.Size = VAL32(m_dwStrongNameSize);

    m_corHeader->VTableFixups.VirtualAddress = VAL32(m_dwVTableRVA);
    m_corHeader->VTableFixups.Size = VAL32(m_dwVTableSize);

    getPEWriter().setCharacteristics(
//#ifndef _WIN64
        IMAGE_FILE_32BIT_MACHINE |
//#endif
        IMAGE_FILE_EXECUTABLE_IMAGE | 
        IMAGE_FILE_LINE_NUMS_STRIPPED | 
        IMAGE_FILE_LOCAL_SYMS_STRIPPED
    );

    m_corHeader->cb = VAL32(sizeof(IMAGE_COR20_HEADER));
    m_corHeader->MajorRuntimeVersion = VAL16(COR_VERSION_MAJOR);
    m_corHeader->MinorRuntimeVersion = VAL16(COR_VERSION_MINOR);
    if (m_dllSwitch)
        getPEWriter().setCharacteristics(IMAGE_FILE_DLL);
    if (m_objSwitch)
        getPEWriter().clearCharacteristics(IMAGE_FILE_DLL | IMAGE_FILE_EXECUTABLE_IMAGE | IMAGE_FILE_LOCAL_SYMS_STRIPPED);
    m_corHeader->Flags = VAL32(m_comImageFlags);
    m_corHeader->EntryPointToken = VAL32(m_entryPoint);
    _ASSERTE(TypeFromToken(m_entryPoint) == mdtMethodDef || m_entryPoint == mdTokenNil || 
             TypeFromToken(m_entryPoint) == mdtFile);
    setDirectoryEntry(getCorHeaderSection(), IMAGE_DIRECTORY_ENTRY_COMHEADER, sizeof(IMAGE_COR20_HEADER), m_corHeaderOffset);

    if ((m_comImageFlags & COMIMAGE_FLAGS_IL_LIBRARY) == 0
        && !m_linked && !m_objSwitch)
    {
        hr = emitExeMain();
        if (FAILED(hr))
            return hr;

    }

    m_linked = true;

    IfFailRet(getPEWriter().link());

    return S_OK;
} // HRESULT CeeFileGenWriter::link()


HRESULT CeeFileGenWriter::fixup()
{
    HRESULT hr;

    m_fixed = true;

    if (!m_linked)
        IfFailRet(link());

    CeeGenTokenMapper *pMapper = getTokenMapper();

    // Apply token remaps if there are any.
    if (! m_fTokenMapSupported && pMapper != NULL) {
        IMetaDataImport *pImport;
        hr = pMapper->GetMetaData(&pImport);
        _ASSERTE(SUCCEEDED(hr));
        hr = MapTokens(pMapper, pImport);
        pImport->Release();

    }

    // remap the entry point if entry point token has been moved
    if (pMapper != NULL && !m_objSwitch) 
    {
        mdToken tk = m_entryPoint;
        pMapper->HasTokenMoved(tk, tk);
        m_corHeader->EntryPointToken = VAL32(tk);
    }

    IfFailRet(getPEWriter().fixup(pMapper)); 

    return S_OK;
} // HRESULT CeeFileGenWriter::fixup()

HRESULT CeeFileGenWriter::generateImage(void **ppImage)
{
    HRESULT hr;

    if (!m_fixed)
        IfFailRet(fixup());

    LPWSTR outputFileName = m_outputFileName;

    if (! outputFileName && ppImage == NULL) {
        if (m_comImageFlags & COMIMAGE_FLAGS_IL_LIBRARY)
            outputFileName = L"output.ill";
        else if (m_dllSwitch)
            outputFileName = L"output.dll";
        else if (m_objSwitch)
            outputFileName = L"output.obj";
        else
            outputFileName = L"output.exe";
    }

    // output file name and ppImage are mutually exclusive
    _ASSERTE((NULL == outputFileName && ppImage != NULL) || (outputFileName != NULL && NULL == ppImage));

    if (outputFileName != NULL)
        IfFailRet(getPEWriter().write(outputFileName));
    else
        IfFailRet(getPEWriter().write(ppImage));

    return S_OK;
} // HRESULT CeeFileGenWriter::generateImage()

HRESULT CeeFileGenWriter::setOutputFileName(LPWSTR fileName)
{
    if (m_outputFileName)
        delete[] m_outputFileName;
    m_outputFileName = (LPWSTR)new WCHAR[(lstrlenW(fileName) + 1)];
    TESTANDRETURN(m_outputFileName!=NULL, E_OUTOFMEMORY);
    wcscpy(m_outputFileName, fileName);
    return S_OK;
} // HRESULT CeeFileGenWriter::setOutputFileName()

HRESULT CeeFileGenWriter::setResourceFileName(LPWSTR fileName)
{
    if (m_resourceFileName)
        delete[] m_resourceFileName;
    m_resourceFileName = (LPWSTR)new WCHAR[(lstrlenW(fileName) + 1)];
    TESTANDRETURN(m_resourceFileName!=NULL, E_OUTOFMEMORY);
    wcscpy(m_resourceFileName, fileName);
    return S_OK;
} // HRESULT CeeFileGenWriter::setResourceFileName()

HRESULT CeeFileGenWriter::setLibraryName(LPWSTR libraryName)
{
    if (m_libraryName)
        delete[] m_libraryName;
    m_libraryName = (LPWSTR)new WCHAR[(lstrlenW(libraryName) + 1)];
    TESTANDRETURN(m_libraryName != NULL, E_OUTOFMEMORY);
    wcscpy(m_libraryName, libraryName);
    return S_OK;
} // HRESULT CeeFileGenWriter::setLibraryName()

HRESULT CeeFileGenWriter::setLibraryGuid(LPWSTR libraryGuid)
{
    return IIDFromString(libraryGuid, &m_libraryGuid);
} // HRESULT CeeFileGenWriter::setLibraryGuid()


HRESULT CeeFileGenWriter::emitLibraryName(IMetaDataEmit *emitter)
{
    HRESULT hr;
    IfFailRet(emitter->SetModuleProps(m_libraryName));
    
    // Set the GUID as a custom attribute, if it is not NULL_GUID.
    if (m_libraryGuid != GUID_NULL)
    {
        static COR_SIGNATURE _SIG[] = INTEROP_GUID_SIG;
        mdTypeRef tr;
        mdMemberRef mr;
        WCHAR wzGuid[40];
        BYTE  rgCA[50];
        IfFailRet(emitter->DefineTypeRefByName(mdTypeRefNil, INTEROP_GUID_TYPE_W, &tr));
        IfFailRet(emitter->DefineMemberRef(tr, L".ctor", _SIG, sizeof(_SIG), &mr));
        StringFromGUID2(m_libraryGuid, wzGuid, lengthof(wzGuid));
        memset(rgCA, 0, sizeof(rgCA));
        // Tag is 0x0001
        rgCA[0] = 1;
        // Length of GUID string is 36 characters.
        rgCA[2] = 0x24;
        // Convert 36 characters, skipping opening {, into 3rd byte of buffer.
        WszWideCharToMultiByte(CP_ACP,0, wzGuid+1,36, reinterpret_cast<char*>(&rgCA[3]),36, 0,0);
        hr = emitter->DefineCustomAttribute(1,mr,rgCA,41,0);
    }
    return (hr);
} // HRESULT CeeFileGenWriter::emitLibraryName()

HRESULT CeeFileGenWriter::setImageBase(size_t imageBase) 
{
    getPEWriter().setImageBase(imageBase);
    return S_OK;
} // HRESULT CeeFileGenWriter::setImageBase()

HRESULT CeeFileGenWriter::setFileAlignment(ULONG fileAlignment) 
{
    getPEWriter().setFileAlignment(fileAlignment);
    return S_OK;
} // HRESULT CeeFileGenWriter::setFileAlignment()

HRESULT CeeFileGenWriter::setSubsystem(DWORD subsystem, DWORD major, DWORD minor)
{
    getPEWriter().setSubsystem(subsystem, major, minor);
    return S_OK;
} // HRESULT CeeFileGenWriter::setSubsystem()

HRESULT CeeFileGenWriter::checkForErrors()
{
    if (TypeFromToken(m_entryPoint) == mdtMethodDef) {
        if (m_dllSwitch) {
//          }
        } 
        return S_OK;
    }
    return S_OK;
} // HRESULT CeeFileGenWriter::checkForErrors()

HRESULT CeeFileGenWriter::getMethodRVA(ULONG codeOffset, ULONG *codeRVA)
{
    _ASSERTE(codeRVA);
    *codeRVA = getPEWriter().getIlRva() + codeOffset;
    return S_OK;
} // HRESULT CeeFileGenWriter::getMethodRVA()

HRESULT CeeFileGenWriter::setDirectoryEntry(CeeSection &section, ULONG entry, ULONG size, ULONG offset)
{
    return getPEWriter().setDirectoryEntry((PEWriterSection*)(&section.getImpl()), entry, size, offset);
} // HRESULT CeeFileGenWriter::setDirectoryEntry()

HRESULT CeeFileGenWriter::getFileTimeStamp(time_t *pTimeStamp)
{
    return getPEWriter().getFileTimeStamp(pTimeStamp);
} // HRESULT CeeFileGenWriter::getFileTimeStamp()

HRESULT CeeFileGenWriter::setAddrReloc(UCHAR *instrAddr, DWORD value)
{
	*(DWORD *)instrAddr = VAL32(value);
    return S_OK;
} // HRESULT CeeFileGenWriter::setAddrReloc()

HRESULT CeeFileGenWriter::addAddrReloc(CeeSection &thisSection, UCHAR *instrAddr, DWORD offset, CeeSection *targetSection)
{
    if (!targetSection) {
        thisSection.addBaseReloc(offset, srRelocHighLow);
    } else {
        thisSection.addSectReloc(offset, *targetSection, srRelocHighLow);
    }
    return S_OK;
} // HRESULT CeeFileGenWriter::addAddrReloc()

// create ExeMain and import directory into .text and the .iat into .data
//
// The structure of the import directory information is as follows, but it is not contiguous in 
// section. All the r/o data goes into the .text section and the iat array (which the loader
// updates with the imported addresses) goes into the .data section because WINCE needs it to be writable.
//
//    struct IData {
//      // one for each DLL, terminating in NULL
//      IMAGE_IMPORT_DESCRIPTOR iid[];      
//      // import lookup table: a set of entries for the methods of each DLL, 
//      // terminating each set with NULL
//      IMAGE_THUNK_DATA ilt[];
//      // hint/name table: an set of entries for each method of each DLL wiht
//      // no terminating entry
//      struct {
//          WORD Hint;
//          // null terminated string
//          BYTE Name[];
//      } ibn;      // Hint/name table
//      // import address table: a set of entries for the methods of each DLL, 
//      // terminating each set with NULL
//      IMAGE_THUNK_DATA iat[];
//      // one for each DLL, null terminated strings
//      BYTE DllName[];
//  };
//

// IAT must be first in its section, so have code here to allocate it up front
// prior to knowing other info such as if dll or not. This won't work if have > 1
// function imported, but we'll burn that bridge when we get to it.
HRESULT CeeFileGenWriter::allocateIAT()
{
    m_dllCount = 1;
    m_iDataDlls = (IDataDllInfo *)malloc(m_dllCount * sizeof(IDataDllInfo));
    if (m_iDataDlls == 0) {
        return E_OUTOFMEMORY;
    }
    memset(m_iDataDlls, '\0', m_dllCount * sizeof(IDataDllInfo));
    m_iDataDlls[0].m_name = "mscoree.dll";
    m_iDataDlls[0].m_numMethods = 1;
    m_iDataDlls[0].m_methodName = 
                (char **)malloc(m_iDataDlls[0].m_numMethods * sizeof(char *));
    if (! m_iDataDlls[0].m_methodName) {
        return E_OUTOFMEMORY;
    }
    m_iDataDlls[0].m_methodName[0] = NULL;

    int iDataSizeIAT = 0;

    for (int i=0; i < m_dllCount; i++) {
        m_iDataDlls[i].m_iatOffset = iDataSizeIAT;
        iDataSizeIAT += (m_iDataDlls[i].m_numMethods + 1) * sizeof(IMAGE_THUNK_DATA);
    }

    HRESULT hr = getSectionCreate(".text0", sdExecute, &m_iDataSectionIAT);
    TESTANDRETURNHR(hr);
    m_iDataOffsetIAT = m_iDataSectionIAT->dataLen();
    _ASSERTE(m_iDataOffsetIAT == 0);
    m_iDataIAT = m_iDataSectionIAT->getBlock(iDataSizeIAT);
    if (! m_iDataIAT) {
        return E_OUTOFMEMORY;
    }
    memset(m_iDataIAT, '\0', iDataSizeIAT);

    // Don't set the IAT directory entry yet, since we may not actually end up doing
    // an emitExeMain.

    return S_OK;
} // HRESULT CeeFileGenWriter::allocateIAT()

HRESULT CeeFileGenWriter::emitExeMain()
{
    // Note: code later on in this method assumes that mscoree.dll is at
    // index m_iDataDlls[0], with CorDllMain or CorExeMain at method[0]

    if (m_dllSwitch) {
        m_iDataDlls[0].m_methodName[0] = "_CorDllMain";
    } else {
        m_iDataDlls[0].m_methodName[0] = "_CorExeMain";
    }

    int iDataSizeIAT = 0;
    int iDataSizeRO = (m_dllCount + 1) * sizeof(IMAGE_IMPORT_DESCRIPTOR);
    CeeSection &iDataSectionRO = getTextSection();
    int iDataOffsetRO = iDataSectionRO.dataLen();
    int i;
    for (i=0; i < m_dllCount; i++) {
        m_iDataDlls[i].m_iltOffset = iDataSizeRO + iDataSizeIAT;
        iDataSizeIAT += (m_iDataDlls[i].m_numMethods + 1) * sizeof(IMAGE_THUNK_DATA);
    }

    iDataSizeRO += iDataSizeIAT;

    for (i=0; i < m_dllCount; i++) {
        int delta = (iDataSizeRO + iDataOffsetRO) % 16;
        // make sure is on a 16-byte offset
        if (delta != 0)
            iDataSizeRO += (16 - delta);
        _ASSERTE((iDataSizeRO + iDataOffsetRO) % 16 == 0);
        m_iDataDlls[i].m_ibnOffset = iDataSizeRO;
        for (int j=0; j < m_iDataDlls[i].m_numMethods; j++) {
            int nameLen = (int)(strlen(m_iDataDlls[i].m_methodName[j]) + 1);
            iDataSizeRO += sizeof(WORD) + nameLen + nameLen%2;
        }
    }
    for (i=0; i < m_dllCount; i++) {
        m_iDataDlls[i].m_nameOffset = iDataSizeRO;
        iDataSizeRO += (int)(strlen(m_iDataDlls[i].m_name) + 2);
    }                                                             

    char *iDataRO = iDataSectionRO.getBlock(iDataSizeRO);
    if (! iDataRO) {
        return E_OUTOFMEMORY;
    }
    memset(iDataRO, '\0', iDataSizeRO);

    setDirectoryEntry(iDataSectionRO, IMAGE_DIRECTORY_ENTRY_IMPORT, iDataSizeRO, iDataOffsetRO);

    IMAGE_IMPORT_DESCRIPTOR *iid = (IMAGE_IMPORT_DESCRIPTOR *)iDataRO;        
    for (i=0; i < m_dllCount; i++) {

        // fill in the import descriptors for each DLL
        IMAGE_IMPORT_DESC_FIELD(iid[i], OriginalFirstThunk) = VAL32((ULONG)(m_iDataDlls[i].m_iltOffset + iDataOffsetRO));
        iid[i].Name = VAL32(m_iDataDlls[i].m_nameOffset + iDataOffsetRO);
        iid[i].FirstThunk = VAL32((ULONG)(m_iDataDlls[i].m_iatOffset + m_iDataOffsetIAT));

        iDataSectionRO.addSectReloc(
            (unsigned)(iDataOffsetRO + (char *)(&IMAGE_IMPORT_DESC_FIELD(iid[i], OriginalFirstThunk)) - iDataRO), iDataSectionRO, srRelocAbsolute);
        iDataSectionRO.addSectReloc(
            (unsigned)(iDataOffsetRO + (char *)(&iid[i].Name) - iDataRO), iDataSectionRO, srRelocAbsolute);
        iDataSectionRO.addSectReloc(
            (unsigned)(iDataOffsetRO + (char *)(&iid[i].FirstThunk) - iDataRO), *m_iDataSectionIAT, srRelocAbsolute);

        // now fill in the import lookup table for each DLL
        IMAGE_THUNK_DATA *ilt = (IMAGE_THUNK_DATA*)
                        (iDataRO + m_iDataDlls[i].m_iltOffset);
        IMAGE_THUNK_DATA *iat = (IMAGE_THUNK_DATA*)
                        (m_iDataIAT + m_iDataDlls[i].m_iatOffset);

        int ibnOffset = m_iDataDlls[i].m_ibnOffset;
        for (int j=0; j < m_iDataDlls[i].m_numMethods; j++) 
        {
            SET_UNALIGNED_VAL32(&ilt[j].u1.AddressOfData, (ULONG_PTR)(ibnOffset + iDataOffsetRO));
            SET_UNALIGNED_VAL32(&iat[j].u1.AddressOfData, (ULONG_PTR)(ibnOffset + iDataOffsetRO));

            iDataSectionRO.addSectReloc(
                (unsigned)(iDataOffsetRO + (char *)(&ilt[j].u1.AddressOfData) - iDataRO), iDataSectionRO, srRelocAbsolute);
            m_iDataSectionIAT->addSectReloc(
                (unsigned)(m_iDataOffsetIAT + (char *)(&iat[j].u1.AddressOfData) - m_iDataIAT), iDataSectionRO, srRelocAbsolute);
            int nameLen = (int)(strlen(m_iDataDlls[i].m_methodName[j]) + 1);
            memcpy(iDataRO + ibnOffset + offsetof(IMAGE_IMPORT_BY_NAME, Name), 
                                    m_iDataDlls[i].m_methodName[j], nameLen);
            ibnOffset += sizeof(WORD) + nameLen + nameLen%2;
        }

        // now fill in the import lookup table for each DLL
        strcpy(iDataRO + m_iDataDlls[i].m_nameOffset, m_iDataDlls[i].m_name);
    };

    // Put the entry point code into the PE file
    unsigned entryPointOffset = getTextSection().dataLen();
    int iatOffset = (int)(entryPointOffset + (m_dllSwitch ? CorDllMainIATOffset : CorExeMainIATOffset));
    const int align = 4;
    // WinCE needs to have the IAT offset on a 4-byte boundary because it will be loaded and fixed up even
    // for RISC platforms, where DWORDs must be 4-byte aligned.  So compute number of bytes to round up by 
    // to put iat offset on 4-byte boundary
    int diff = ((iatOffset + align -1) & ~(align-1)) - iatOffset;
    if (diff) {
        // force to 4-byte boundary
        if(NULL==getTextSection().getBlock(diff)) return E_OUTOFMEMORY;
        entryPointOffset += diff;
    }
    _ASSERTE((getTextSection().dataLen() + (m_dllSwitch ? CorDllMainIATOffset : CorExeMainIATOffset)) % 4 == 0);

    getPEWriter().setEntryPointTextOffset(entryPointOffset);
    if (m_dllSwitch) {
        UCHAR *dllMainBuf = (UCHAR*)getTextSection().getBlock(sizeof(DllMainTemplate));
        if(dllMainBuf==NULL) return E_OUTOFMEMORY;
        memcpy(dllMainBuf, DllMainTemplate, sizeof(DllMainTemplate));
        //mscoree.dll
        setAddrReloc(dllMainBuf+CorDllMainIATOffset, m_iDataDlls[0].m_iatOffset + m_iDataOffsetIAT);
        addAddrReloc(getTextSection(), dllMainBuf, entryPointOffset+CorDllMainIATOffset, m_iDataSectionIAT);
    } else {
        UCHAR *exeMainBuf = (UCHAR*)getTextSection().getBlock(sizeof(ExeMainTemplate));
        if(exeMainBuf==NULL) return E_OUTOFMEMORY;
        memcpy(exeMainBuf, ExeMainTemplate, sizeof(ExeMainTemplate));
        //mscoree.dll
        setAddrReloc(exeMainBuf+CorExeMainIATOffset, m_iDataDlls[0].m_iatOffset + m_iDataOffsetIAT);
        addAddrReloc(getTextSection(), exeMainBuf, entryPointOffset+CorExeMainIATOffset, m_iDataSectionIAT);
    }

    // Now set our IAT entry since we're using the IAT
    setDirectoryEntry(*m_iDataSectionIAT, IMAGE_DIRECTORY_ENTRY_IAT, iDataSizeIAT, m_iDataOffsetIAT);

    return S_OK;
} // HRESULT CeeFileGenWriter::emitExeMain()


HRESULT CeeFileGenWriter::setManifestEntry(ULONG size, ULONG offset)
{
    if (offset)
        m_dwManifestRVA = offset;
    else {
        CeeSection TextSection = getTextSection();
        getMethodRVA(TextSection.dataLen() - size, &m_dwManifestRVA);
    }

    m_dwManifestSize = size;
    return S_OK;
} // HRESULT CeeFileGenWriter::setManifestEntry()

HRESULT CeeFileGenWriter::setStrongNameEntry(ULONG size, ULONG offset)
{
    m_dwStrongNameRVA = offset;
    m_dwStrongNameSize = size;
    return S_OK;
} // HRESULT CeeFileGenWriter::setStrongNameEntry()

HRESULT CeeFileGenWriter::setVTableEntry(ULONG size, ULONG offset)
{
    if (offset && size)
    {
		void * pv;
        CeeSection TextSection = getTextSection();
        getMethodRVA(TextSection.dataLen(), &m_dwVTableRVA);
        if((pv = TextSection.getBlock(size)))
			memcpy(pv,(void *)offset,size);
		else return E_OUTOFMEMORY;
        m_dwVTableSize = size;
    }

    return S_OK;
} // HRESULT CeeFileGenWriter::setVTableEntry()

HRESULT CeeFileGenWriter::setEnCRvaBase(ULONG dataBase, ULONG rdataBase)
{
    setEnCMode();
    getPEWriter().setEnCRvaBase(dataBase, rdataBase);
    return S_OK;
} // HRESULT CeeFileGenWriter::setEnCRvaBase()

HRESULT CeeFileGenWriter::computeSectionOffset(CeeSection &section, char *ptr,
                                               unsigned *offset)
{
    *offset = section.computeOffset(ptr);

    return S_OK;
} // HRESULT CeeFileGenWriter::computeSectionOffset()

HRESULT CeeFileGenWriter::computeOffset(char *ptr,
                                        CeeSection **pSection, unsigned *offset)
{
    TESTANDRETURNPOINTER(pSection);

    CeeSection **s = m_sections;
    CeeSection **sEnd = s + m_numSections;
    while (s < sEnd)
    {
        if ((*s)->containsPointer(ptr))
        {
            *pSection = *s;
            *offset = (*s)->computeOffset(ptr);

            return S_OK;
        }
        s++;
    }

    return E_FAIL;
} // HRESULT CeeFileGenWriter::computeOffset()

HRESULT CeeFileGenWriter::getCorHeader(IMAGE_COR20_HEADER **ppHeader)
{
    *ppHeader = m_corHeader;
    return S_OK;
} // HRESULT CeeFileGenWriter::getCorHeader()

// Globals.
HINSTANCE       g_hThisInst;            // This library.

//*****************************************************************************
// Handle lifetime of loaded library.
//*****************************************************************************
extern "C"
BOOL WINAPI DllMain(HANDLE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        {   // Save the module handle.            
            g_hThisInst = (HINSTANCE)hInstance;
            DisableThreadLibraryCalls((HMODULE)hInstance);
        }
        break;
    }

    return (true);
} // BOOL WINAPI DllMain()


HINSTANCE GetModuleInst()
{
    return (g_hThisInst);
} // HINSTANCE GetModuleInst()

