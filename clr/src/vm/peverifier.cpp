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
/*
 *
 * Header:  PEVerifier.cpp
 *
 *                                     
 *
 * Purpose: Verify PE images before loading. This is to prevent
 *          Native code (other than Mscoree.DllMain() ) to execute.
 *
 * The entry point should be the following instruction.
 *
 * [_X86_]
 * jmp dword ptr ds:[XXXX]
 *
 * XXXX should be the RVA of the first entry in the IAT.
 * The IAT should have only one entry, that should be MSCoree.dll:_CorMain
 *
 * Date created : 1 July 1999 
 * 
 */

#ifdef _PEVERIFIER_EXE_
#include <windows.h>
#include <crtdbg.h>
#endif
#include "common.h"
#include "peverifier.h"

// See if the bitmap's i'th bit 0 ==> 1st bit, 1 ==> 2nd bit...
#define IS_SET_DWBITMAP(bitmap, i) ( ((i) > 31) ? 0 : ((bitmap) & (1 << (i))) )

#ifdef _X86_
// jmp dword ptr ds:[XXXX]
#define JMP_DWORD_PTR_DS_OPCODE { 0xFF, 0x25 }   
#define JMP_DWORD_PTR_DS_OPCODE_SIZE   2        // Size of opcode
#define JMP_SIZE   6                            // Size of opcode + operand
#endif

#define CLR_MAX_RVA 0x80000000L

#ifdef _MODULE_VERIFY_LOG 
void PEVerifier::LogError(PCHAR szField, DWORD dwActual, DWORD *pdwExpected, int n)
{
    Log(szField); Log(" = "); Log(dwActual);

    Log(" [ Expected "); 

    for (int i=0; i<n; ++i)
    {
        if (i == 0)
            Log("-> ");
        else if (i == n - 1)
            Log(" or ");
        else
            Log(", ");
        Log(pdwExpected[i]);
    }

    Log(" ]");
    Log("\n");
}

void PEVerifier::LogError(PCHAR szField, DWORD dwActual, DWORD dwExpected)
{
    Log(szField); Log(" = "); Log(dwActual);
    Log(" [ Expected "); Log(dwExpected); Log(" ]");
    Log("\n");
}
#else
#define Log(a) 
#define LogError(a, b, c) 
#endif

#define CHECK_HEADER(p, Struct, name)  {                            \
    if (p == NULL)                                                  \
    {                                                               \
        Log(name); Log(" is NULL\n");                               \
        return FALSE;                                               \
    }                                                               \
                                                                    \
    if (((PBYTE)p + sizeof(Struct)) > (m_pBase + m_dwLength))       \
    {                                                               \
        Log(name); Log(" incomplete\n");                            \
        return FALSE;                                               \
    }                                                               \
}

#define ALIGN(v, a) (((v)+(a)-1)&~((a)-1))

#define CHECK_ALIGNMENT_VALIDITY(a, name)                           \
    if (((a)&((a)-1)) != 0)                                         \
    {                                                               \
        Log("Bad alignment value ");                                \
        Log(name);                                                  \
        return FALSE;                                               \
    }

#define CHECK_ALIGNMENT(v, a, name)                                 \
    if (((v)&((a)-1)) != 0)                                         \
    {                                                               \
        Log("Improperly aligned value ");                           \
        Log(name);                                                  \
        return FALSE;                                               \
    }

BOOL PEVerifier::Check()
{
#define CHECK(x) if ((ret = Check##x()) == FALSE) goto Exit;

#define CHECK_OVERFLOW(offs) {                                      \
    if (offs & CLR_MAX_RVA)                                         \
    {                                                               \
        Log("overflow\n");                                          \
        ret = FALSE;                                                \
        goto Exit;                                                  \
    }                                                               \
}


    BOOL ret = TRUE;
    m_pDOSh = (PIMAGE_DOS_HEADER)m_pBase;
    CHECK(DosHeader);

    CHECK_OVERFLOW(VAL32(m_pDOSh->e_lfanew));
    m_pNTh = (PIMAGE_NT_HEADERS) (m_pBase + VAL32(m_pDOSh->e_lfanew));
    CHECK(NTHeader);

    m_pFh = (PIMAGE_FILE_HEADER) &(m_pNTh->FileHeader);
    CHECK(FileHeader);

    m_nSections = VAL16(m_pFh->NumberOfSections);

    m_pOPTh = (PIMAGE_OPTIONAL_HEADER) &(m_pNTh->OptionalHeader);
    CHECK(OptionalHeader);

    m_dwPrefferedBase = VAL32(m_pOPTh->ImageBase);

    CHECK_OVERFLOW(VAL16(m_pFh->SizeOfOptionalHeader));
    m_pSh = (PIMAGE_SECTION_HEADER) ( (PBYTE)m_pOPTh + 
            VAL16(m_pFh->SizeOfOptionalHeader));

    CHECK(SectionHeader);
    
    CHECK(Directories);

    CHECK(ImportDlls);
    _ASSERTE(m_dwIATRVA);

    CHECK(Relocations);

	CHECK(COMHeader);

    CHECK(EntryPoint);

Exit:
    return ret;

#undef CHECK
#undef CHECK_OVERFLOW
}

BOOL PEVerifier::CheckDosHeader() const
{
    CHECK_HEADER(m_pDOSh, IMAGE_DOS_HEADER, "Dos Header");

    if (m_pDOSh->e_magic != VAL16(IMAGE_DOS_SIGNATURE))
    {
        LogError("IMAGE_DOS_HEADER.e_magic", m_pDOSh->e_magic, 
            VAL16(IMAGE_DOS_SIGNATURE));
        return FALSE;
    }

    if (m_pDOSh->e_lfanew == 0)
    {
        LogError("IMAGE_DOS_HEADER.e_lfanew", m_pDOSh->e_lfanew, 0xFFFFFFFF);
        return FALSE;
    }

    return TRUE;
}

BOOL PEVerifier::CheckNTHeader() const
{
    CHECK_HEADER(m_pNTh, PIMAGE_NT_HEADERS, "NT Header");

    if (m_pNTh->Signature != VAL32(IMAGE_NT_SIGNATURE))
    {
        LogError("IMAGE_NT_HEADER.Signature", m_pNTh->Signature, 
            VAL32(IMAGE_NT_SIGNATURE));
        return FALSE;
    }

    return TRUE;
}

BOOL PEVerifier::CheckFileHeader() const
{
    CHECK_HEADER(m_pFh, IMAGE_FILE_HEADER, "File Header");

    if ((VAL16(m_pFh->Characteristics) & IMAGE_FILE_SYSTEM) != 0)
    {
        LogError("IMAGE_FILE_HEADER.Characteristics", m_pFh->Characteristics, 
            (VAL16(m_pFh->Characteristics) & ~IMAGE_FILE_SYSTEM));
        return FALSE;
    }

    if ((m_pFh->SizeOfOptionalHeader != VAL16(sizeof(IMAGE_OPTIONAL_HEADER32)))
        )
    {
#ifdef _MODULE_VERIFY_LOG 

        LogError("IMAGE_FILE_HEADER.SizeOfOptionalHeader", 
                (VAL16(m_pFh->SizeOfOptionalHeader), sizeof(IMAGE_OPTIONAL_HEADER32));

#endif

        return FALSE;
    }

    return TRUE;
}

BOOL PEVerifier::CheckOptionalHeader() const
{
    _ASSERTE(m_pFh != NULL);

    if (m_pFh->SizeOfOptionalHeader == VAL16(sizeof(IMAGE_OPTIONAL_HEADER32))) 
    {
        CHECK_HEADER(m_pOPTh, IMAGE_OPTIONAL_HEADER32, "Optional Header");
    } 
#ifdef _DEBUG
    else _ASSERTE(!"Should never reach here !");
#endif

    if ((m_pOPTh->Magic != VAL16(IMAGE_NT_OPTIONAL_HDR32_MAGIC))
        )
    {
#ifdef _MODULE_VERIFY_LOG 

        LogError("IMAGE_OPTIONAL_HEADER.Magic", VAL16(m_pOPTh->Magic), 
          IMAGE_NT_OPTIONAL_HDR32_MAGIC);

#endif
        return FALSE;
    }

    // Check alignments for validity
    CHECK_ALIGNMENT_VALIDITY(VAL32(m_pOPTh->FileAlignment), "FileAlignment");
    CHECK_ALIGNMENT_VALIDITY(VAL32(m_pOPTh->SectionAlignment), "SectionAlignment");

    // Check alignment values
    CHECK_ALIGNMENT(VAL32(m_pOPTh->FileAlignment), 512, "FileAlignment");
    // NOTE: Our spec requires that managed images have 8K section alignment.  We're not
    // going to enforce this right now since it's not a security issue. (OS_PAGE_SIZE is
    // required for security so proper section protection can be applied.)
    CHECK_ALIGNMENT(VAL32(m_pOPTh->SectionAlignment), OS_PAGE_SIZE, "SectionAlignment");
    CHECK_ALIGNMENT(VAL32(m_pOPTh->SectionAlignment), VAL32(m_pOPTh->FileAlignment), "SectionAlignment");

    // Check that virtual bounds are aligned

    CHECK_ALIGNMENT(VAL32(m_pOPTh->ImageBase), 0x10000 /* 64K */, "ImageBase");
    CHECK_ALIGNMENT(VAL32(m_pOPTh->SizeOfImage), VAL32(m_pOPTh->SectionAlignment), "SizeOfImage");
    CHECK_ALIGNMENT(VAL32(m_pOPTh->SizeOfHeaders), VAL32(m_pOPTh->FileAlignment), "SizeOfHeaders");

    return TRUE;
}

BOOL PEVerifier::CheckSection(DWORD *pOffsetCounter, DWORD dataOffset, DWORD dataSize, 
                              DWORD *pAddressCounter, DWORD virtualAddress, DWORD unalignedVirtualSize,
                              int sectionIndex) const 
{
    // Assert that only one bit is set in this const.
    _ASSERTE( ((CLR_MAX_RVA - 1) & (CLR_MAX_RVA)) == 0);

    DWORD virtualSize = ALIGN(unalignedVirtualSize, VAL32(m_pOPTh->SectionAlignment));

    // Note that since we are checking for the high bit in all of these values, there can be 
    // no overflow when adding any 2 of them.

    if ((dataOffset & CLR_MAX_RVA) ||
        (dataSize & CLR_MAX_RVA) ||
        ((dataOffset + dataSize) & CLR_MAX_RVA) ||
        (virtualAddress & CLR_MAX_RVA) ||
        (virtualSize & CLR_MAX_RVA) ||
        ((virtualAddress + virtualSize) & CLR_MAX_RVA))
    {
        Log("RVA too large for in section ");
        Log(sectionIndex);
        Log("\n");
        return FALSE;
    }

    CHECK_ALIGNMENT(dataOffset, VAL32(m_pOPTh->FileAlignment), "PointerToRawData");
    CHECK_ALIGNMENT(dataSize, VAL32(m_pOPTh->FileAlignment), "SizeOfRawData");
    CHECK_ALIGNMENT(virtualAddress, VAL32(m_pOPTh->SectionAlignment), "VirtualAddress");

    if (dataOffset < *pOffsetCounter
        || dataOffset + dataSize > m_dwLength)
    {
        Log("Bad Section ");
        Log(sectionIndex);
        Log("\n");
        return FALSE;
    }

    if (virtualAddress < *pAddressCounter
        || virtualAddress + virtualSize > VAL32(m_pOPTh->SizeOfImage)
        || dataSize > virtualSize)
    {
        Log("Bad section virtual address in Section ");
        Log(sectionIndex);
        Log("\n");
        return FALSE;
    }

    *pOffsetCounter = dataOffset + dataSize;
    *pAddressCounter = virtualAddress + virtualSize;

    return TRUE;
}


BOOL PEVerifier::CheckSectionHeader() const
{
    _ASSERTE(m_pSh != NULL);

    DWORD lastDataOffset = 0;
    DWORD lastVirtualAddress = 0;
    
    // Check header data as if it were a section
    if (!CheckSection(&lastDataOffset, 0, VAL32(m_pOPTh->SizeOfHeaders), 
                      &lastVirtualAddress, 0, VAL32(m_pOPTh->SizeOfHeaders), 
                      -1))
        return FALSE;

    for (DWORD dw=0; dw<m_nSections; ++dw)
    {
        if ((PBYTE)&m_pSh[dw+1] > m_pBase + VAL32(m_pOPTh->SizeOfHeaders))
        {
            Log("SizeOfHeaders too small\n");
            return FALSE;
        }

        if (!CheckSection(&lastDataOffset, VAL32(m_pSh[dw].PointerToRawData), VAL32(m_pSh[dw].SizeOfRawData),
                          &lastVirtualAddress, VAL32(m_pSh[dw].VirtualAddress), VAL32(m_pSh[dw].Misc.VirtualSize),
                          dw))
            return FALSE;
    }

    return TRUE;
}


BOOL PEVerifier::CheckDirectories() const
{
    _ASSERTE(m_pOPTh != NULL);

    DWORD nEntries = VAL32(m_pOPTh->NumberOfRvaAndSizes);

#ifndef IMAGE_DIRECTORY_ENTRY_COMHEADER
#define IMAGE_DIRECTORY_ENTRY_COMHEADER 14
#endif

    // Allow only verifiable directories.
    //
    // IMAGE_DIRECTORY_ENTRY_IMPORT     1   Import Directory
    // IMAGE_DIRECTORY_ENTRY_RESOURCE   2   Resource Directory
    // IMAGE_DIRECTORY_ENTRY_SECURITY   4   Security Directory
    // IMAGE_DIRECTORY_ENTRY_BASERELOC  5   Base Relocation Table
    // IMAGE_DIRECTORY_ENTRY_DEBUG      6   Debug Directory
    // IMAGE_DIRECTORY_ENTRY_IAT        12  Import Address Table
    //
    // IMAGE_DIRECTORY_ENTRY_COMHEADER  14  COM+ Data
    //
    // Construct a 0 based bitmap with these bits.


    static DWORD s_dwAllowedBitmap = 
        ((1 << (IMAGE_DIRECTORY_ENTRY_IMPORT   )) |
         (1 << (IMAGE_DIRECTORY_ENTRY_RESOURCE )) |
         (1 << (IMAGE_DIRECTORY_ENTRY_SECURITY )) |
         (1 << (IMAGE_DIRECTORY_ENTRY_BASERELOC)) |
         (1 << (IMAGE_DIRECTORY_ENTRY_DEBUG    )) |
         (1 << (IMAGE_DIRECTORY_ENTRY_IAT      )) |
         (1 << (IMAGE_DIRECTORY_ENTRY_COMHEADER)));

    for (DWORD dw=0; dw<nEntries; ++dw)
    {
        if ((m_pOPTh->DataDirectory[dw].VirtualAddress != 0)
            || (m_pOPTh->DataDirectory[dw].Size != 0))
        {
            if (!IS_SET_DWBITMAP(s_dwAllowedBitmap, dw))
            {
                Log("IMAGE_OPTIONAL_HEADER.DataDirectory[");
                Log(dw);
                Log("]");
                Log(" Cannot verify this DataDirectory\n");
                return FALSE;
            }
        }
    }
        
    return TRUE;
}

// This function has a side effect of setting m_dwIATRVA
BOOL PEVerifier::CheckImportDlls()
{
    // The only allowed DLL Imports are MscorEE.dll:_CorExeMain,_CorDllMain

    DWORD dwSectionSize;
    DWORD dwSectionOffset;
    DWORD dwImportSize;
    DWORD dwImportOffset;

    dwImportOffset = DirectoryToOffset(IMAGE_DIRECTORY_ENTRY_IMPORT, 
                            &dwImportSize, &dwSectionOffset, &dwSectionSize);

    // A valid COM+ image should have MscorEE imported.
    if (dwImportOffset == 0)
    {
        Log("IMAGE_DIRECTORY_IMPORT not found.");
        return FALSE;
    }

    // Check if import records will fit into the section that contains it.
    if ((dwSectionOffset + dwSectionSize) < (dwImportOffset + dwImportSize))
    {
ImportSizeError:

        Log("IMAGE_IMPORT_DESCRIPTOR too big to fit in it's section\n");
        return FALSE;
    }

    // There should be space for atleast 2 entries.
    // The second one being a null entry.
    if (dwImportSize < 2*sizeof(IMAGE_IMPORT_DESCRIPTOR))
        goto ImportSizeError;

    PIMAGE_IMPORT_DESCRIPTOR pID = (PIMAGE_IMPORT_DESCRIPTOR)
        ((PBYTE)m_pBase + dwImportOffset);

    // There are no entries in this table.
    if (IMAGE_IMPORT_DESC_FIELD(pID[0], Characteristics) == 0)
    {
        Log("IMAGE_IMPORT_DESCRIPTOR[0] missing MscorEE.dll\n");
        return FALSE;
    }

    // There should be only one entry in this table.
    if (IMAGE_IMPORT_DESC_FIELD(pID[1], Characteristics) != 0)
    {
        Log("IMAGE_IMPORT_DESCRIPTOR[1] should be NULL\n");
        return FALSE;
    }

    // Should never be forwarded
    if ((pID[0].ForwarderChain != 0) && (VAL32(pID[0].ForwarderChain) != (DWORD)-1))
    {
        Log("IMAGE_IMPORT_DESCRIPTOR[0], forwarding not allowed\n");
        return FALSE;
    }

    // Check if mscoree.dll is the import dll name.
    static const CHAR s_szDllName[] = "mscoree.dll";
#define LENGTH_OF_DLL_NAME 11

#ifdef _DEBUG
    _ASSERTE(strlen(s_szDllName) == LENGTH_OF_DLL_NAME);
#endif

    if (CompareStringAtRVA(VAL32(pID[0].Name), (CHAR*)s_szDllName, LENGTH_OF_DLL_NAME) 
        == FALSE)
    {
#ifdef _MODULE_VERIFY_LOG 
        DWORD dwNameOffset = RVAToOffset(VAL32(pID[0].Name), NULL, NULL);
        Log("IMAGE_IMPORT_DESCRIPTOR[0], cannot import library");

        if (dwNameOffset)
        {
            CHAR *pChar = (CHAR*) (m_pBase + dwNameOffset);
            Log(" ");
            Log(pChar);
        }

        Log("\n");
#endif
        return FALSE;
    }

    // Check the Hint/Name table and Import Address table.
    // They could be the same.

    if (CheckImportByNameTable(VAL32(IMAGE_IMPORT_DESC_FIELD(pID[0], OriginalFirstThunk)), FALSE) == FALSE)
    {
        Log("IMAGE_IMPORT_DESCRIPTOR[0].OriginalFirstThunk bad.\n");
        return FALSE;
    }

    // IAT need to be checked only for size.
    if ((IMAGE_IMPORT_DESC_FIELD(pID[0], OriginalFirstThunk) != pID[0].FirstThunk) &&
        (CheckImportByNameTable(VAL32(pID[0].FirstThunk), TRUE) == FALSE))
    {
        Log("IMAGE_IMPORT_DESCRIPTOR[0].FirstThunk bad.\n");
        return FALSE;
    }

    // Cache the RVA of the Import Address Table.
    // For Performance reasons, no seperate function to do this.
    m_dwIATRVA = VAL32(pID[0].FirstThunk);

    return TRUE;
}

// This function has a side effect of setting m_dwRelocRVA (the RVA where a 
// reloc will be applied.)
BOOL PEVerifier::CheckRelocations()
{
    DWORD dwSectionSize;
    DWORD dwSectionOffset;
    DWORD dwRelocSize;
    DWORD dwRelocOffset;

    dwRelocOffset = DirectoryToOffset(IMAGE_DIRECTORY_ENTRY_BASERELOC, 
                        &dwRelocSize, &dwSectionOffset, &dwSectionSize);

    // No reloc is OK for .exes & .dll with IMAGE_FILE_RELOCS_STRIPPED.
    if (dwRelocOffset == 0)
        return TRUE;

    // Check if import records will fit into the section that contains it.
    if ((dwSectionOffset + dwSectionSize) < (dwRelocOffset + dwRelocSize))
    {
        Log("IMAGE_BASE_RELOCATION too big to fit in it's section\n");
        return FALSE;
    }

    // There should be at least one entry.
    if (dwRelocSize < sizeof(IMAGE_BASE_RELOCATION) + sizeof(WORD))
    {
        Log("IMAGE_DIRECTORY_ENTRY_IMPORT.Size Expect greater than ");
        Log((DWORD) sizeof(IMAGE_BASE_RELOCATION));
        Log(" Found ");
        Log(dwRelocSize);
        Log("\n");
        return FALSE;
    }

    IMAGE_BASE_RELOCATION *pReloc = (IMAGE_BASE_RELOCATION *)
        ((PBYTE)m_pBase + dwRelocOffset);

    // Only one Reloc record is expected
    if (VAL32(pReloc->SizeOfBlock) != dwRelocSize)
    {
        Log("IMAGE_BASE_RELOCATION.SizeOfBlock One record Expected ");
        Log(dwRelocSize);
        Log(" Found ");
        Log(VAL32(pReloc->SizeOfBlock));
        Log("\n");
        return FALSE;
    }

    // Check if the size field is correct.
    if (VAL32(pReloc->SizeOfBlock) < sizeof(IMAGE_BASE_RELOCATION))
    {
        Log("IMAGE_BASE_RELOCATION[0].SizeOfBlock is bad. Expect ");
        Log((DWORD) sizeof(IMAGE_BASE_RELOCATION));
        Log(" Found ");
        Log(VAL32(pReloc->SizeOfBlock));
        Log("\n");
        return FALSE;
    }

    WORD *pwReloc = (WORD *) ((PBYTE)m_pBase + dwRelocOffset + sizeof(IMAGE_BASE_RELOCATION));

    DWORD nEntries = (VAL32(pReloc->SizeOfBlock) - sizeof(IMAGE_BASE_RELOCATION)) 
                             / sizeof(WORD);

    // It is OK to have all null entries
    // There should be one and only one non null entry
    BOOL fFoundOne = FALSE;
    for (DWORD dw=0; dw<nEntries; ++dw)
    {

        // Skip NULL entries
        if ((VAL16(pwReloc[dw]) & 0xF000) != (IMAGE_REL_BASED_ABSOLUTE << 12))
        {
            if (fFoundOne)
            {
                Log("IMAGE_BASE_RELOCATION[0] only one relocation allowed\n");
                return FALSE;
            }

            if ((VAL16(pwReloc[dw]) & 0xF000) != (IMAGE_REL_BASED_HIGHLOW << 12))
            {
                Log("IMAGE_BASE_RELOCATION[0] only HIGHLOW allowed\n");
                return FALSE;
            }

            m_dwRelocRVA = VAL32(pReloc->VirtualAddress) + 
                (VAL16(pwReloc[dw]) & 0x0FFF);

            fFoundOne = TRUE;
        }
    }

    return TRUE;
}

BOOL PEVerifier::CheckEntryPoint() const
{
    _ASSERTE(m_pOPTh != NULL);

    DWORD dwSectionOffset;
    DWORD dwSectionSize;
    DWORD dwOffset;

    if (m_pOPTh->AddressOfEntryPoint == 0)
    {
        Log("PIMAGE_OPTIONAL_HEADER.AddressOfEntryPoint Missing Entry point\n");
        return FALSE;
    }

    dwOffset = RVAToOffset(VAL32(m_pOPTh->AddressOfEntryPoint), 
        &dwSectionOffset, &dwSectionSize);

    if (dwOffset == 0)
    {
        Log("Bad Entry point\n");
        return FALSE;
    }

    // EntryPoint should be a jmp dword ptr ds:[XXXX] instruction.
    // XXXX should be RVA of the first and only entry in the IAT.

#ifdef _X86_
    static BYTE s_DllOrExeMain[] = JMP_DWORD_PTR_DS_OPCODE;

    // First check if we have enough space to hold 2 DWORDS
    if ((dwOffset + JMP_SIZE) > (dwSectionOffset + dwSectionSize))
    {
        Log("Entry Function incomplete\n");
        return FALSE;
    }
#endif // _X86_

#ifdef _X86_
    if (memcmp(m_pBase + dwOffset, s_DllOrExeMain, 
        JMP_DWORD_PTR_DS_OPCODE_SIZE)  != 0)
    {
        Log("Non Verifiable native code in entry stub. Expect ");
        Log(*(WORD*)(s_DllOrExeMain));
        Log(" Found ");
        Log(*(WORD*)((PBYTE)m_pBase+dwOffset));
        Log("\n");
        return FALSE;
    }

    // The operand for the jmp instruction is the RVA of IAT
    // (since we verified that there is one and only one entry in the IAT).
    DWORD dwJmpOperand = m_dwIATRVA + m_dwPrefferedBase;

    if (memcmp(m_pBase + dwOffset + JMP_DWORD_PTR_DS_OPCODE_SIZE,
        (PBYTE)&dwJmpOperand, JMP_SIZE - JMP_DWORD_PTR_DS_OPCODE_SIZE) != 0)
    {
        Log("Non Verifiable native code in entry stub. Expect ");
        Log(dwJmpOperand);
        Log(" Found ");
        Log(*(DWORD*)((PBYTE)m_pBase+dwOffset+JMP_DWORD_PTR_DS_OPCODE_SIZE));
        Log("\n");
        return FALSE;
    }

    // Check for Relocation 
    if (m_dwRelocRVA != 0)
    {
        // Relocation should happen at the RVA of the operand of the 
        // jmp instruction
        if (m_dwRelocRVA != VAL32(m_pOPTh->AddressOfEntryPoint) + 
            JMP_DWORD_PTR_DS_OPCODE_SIZE)
        {
            Log("Relocation not allowed at RVA ");
            Log(m_dwRelocRVA);
            Log("\n");
            return FALSE;
        }
    }
#endif // _X86_

    return TRUE;
}

BOOL PEVerifier::CheckImportByNameTable(DWORD dwRVA, BOOL fIAT) const
{
    if (dwRVA == 0)
        return FALSE;

    DWORD dwSectionOffset;
    DWORD dwSectionSize;
    DWORD dwOffset;
    
    dwOffset = RVAToOffset(dwRVA, &dwSectionOffset, &dwSectionSize);

    if (dwOffset == 0)
        return FALSE;

    // First check if we have enough space to hold 2 DWORDS
    if ((dwOffset + 2 * sizeof(DWORD)) > (dwSectionOffset + dwSectionSize))
        return FALSE;

    // IAT need not be verified. It will be over written by the loader.
    if (fIAT)
        return TRUE;

    DWORD *pImportArray = (DWORD*) ((PBYTE)m_pBase + dwOffset); 

    if (pImportArray[0] == 0)
    {
ErrorImport:

        Log("_CorExeMain OR _CorDllMain should be the one and only import\n");
        return FALSE;
    }

    if (pImportArray[1] != 0)
        goto ErrorImport;

    // First bit Set implies Ordinal lookup
    if (pImportArray[0] & VAL32(0x80000000))
    {
        Log("Mscoree.dll:_CorExeMain/_CorDllMain ordinal lookup not allowed\n");
        return FALSE;
    }

    dwOffset = RVAToOffset(VAL32(pImportArray[0]), &dwSectionOffset, &dwSectionSize);

    if (dwOffset == 0)
        return FALSE;

    static CHAR *s_pEntry1 = "_CorDllMain";
    static CHAR *s_pEntry2 = "_CorExeMain";
#define LENGTH_OF_ENTRY_NAME 11

#ifdef _DEBUG
    _ASSERTE(strlen(s_pEntry1) == LENGTH_OF_ENTRY_NAME);
    _ASSERTE(strlen(s_pEntry2) == LENGTH_OF_ENTRY_NAME);
#endif

    // First check if we have enough space to hold 4 bytes + 
    // _CorExeMain or _CorDllMain and a NULL char

    if ((dwOffset + 4 + LENGTH_OF_ENTRY_NAME + 1) >
        (dwSectionOffset + dwSectionSize))
        return FALSE;

    PIMAGE_IMPORT_BY_NAME pImport = (PIMAGE_IMPORT_BY_NAME) 
        ((PBYTE)m_pBase + dwOffset); 

    // Include the null char when comparing.
    if ((_strnicmp(s_pEntry1, (CHAR*)pImport->Name, LENGTH_OF_ENTRY_NAME+1)!=0)&&
        (_strnicmp(s_pEntry2, (CHAR*)pImport->Name, LENGTH_OF_ENTRY_NAME+1)!=0))
    {
        Log("Attempt to import ");
        Log(pImport->Name);
        Log("\n");
        goto ErrorImport;
    }

    return TRUE;
}

BOOL PEVerifier::CompareStringAtRVA(DWORD dwRVA, CHAR *pStr, DWORD dwSize) const
{
    DWORD dwSectionOffset = 0;
    DWORD dwSectionSize   = 0;
    DWORD dwStringOffset  = RVAToOffset(dwRVA,&dwSectionOffset,&dwSectionSize);

    // First check if we have enough space to hold the string
    if ((dwStringOffset + dwSize) > (dwSectionOffset + dwSectionSize))
        return FALSE;

    // Compare should include the NULL char
    if (_strnicmp(pStr, (CHAR*)(m_pBase + dwStringOffset), dwSize + 1) != 0)
        return FALSE;

    return TRUE;
}

DWORD PEVerifier::RVAToOffset(DWORD dwRVA,
                              DWORD *pdwSectionOffset, 
                              DWORD *pdwSectionSize) const
{
    _ASSERTE(m_pSh != NULL);

    // Find the section that contains the RVA
    for (DWORD dw=0; dw<m_nSections; ++dw)
    {
        if ((VAL32(m_pSh[dw].VirtualAddress) <= dwRVA) &&
            (VAL32(m_pSh[dw].VirtualAddress) + VAL32(m_pSh[dw].SizeOfRawData) > dwRVA))
        {
            if (pdwSectionOffset != NULL)
                *pdwSectionOffset = VAL32(m_pSh[dw].PointerToRawData);

            if (pdwSectionSize != NULL)
                *pdwSectionSize = VAL32(m_pSh[dw].SizeOfRawData);

            return (VAL32(m_pSh[dw].PointerToRawData) +
                    (dwRVA - VAL32(m_pSh[dw].VirtualAddress)));
        }
    }

#ifdef _DEBUG
    if (pdwSectionOffset != NULL)
        *pdwSectionOffset = 0xFFFFFFFF;

    if (pdwSectionSize != NULL)
        *pdwSectionSize = 0xFFFFFFFF;
#endif

    return 0;
}

DWORD PEVerifier::DirectoryToOffset(DWORD dwDirectory, 
                                    DWORD *pdwDirectorySize,
                                    DWORD *pdwSectionOffset, 
                                    DWORD *pdwSectionSize) const
{
    _ASSERTE(m_pOPTh != NULL);

    // Get the Directory RVA from the Optional Header
    if ((dwDirectory >= VAL32(m_pOPTh->NumberOfRvaAndSizes)) || 
        (m_pOPTh->DataDirectory[dwDirectory].VirtualAddress == 0))
    {
#ifdef _DEBUG
        if (pdwDirectorySize != NULL)
            *pdwDirectorySize = 0xFFFFFFFF;

        if (pdwSectionOffset != NULL)
            *pdwSectionOffset = 0xFFFFFFFF;

        if (pdwSectionSize != NULL)
            *pdwSectionSize = 0xFFFFFFFF;
#endif

        return 0;
    }

    if (pdwDirectorySize != NULL)
        *pdwDirectorySize = VAL32(m_pOPTh->DataDirectory[dwDirectory].Size);

    // Return the Offset, using the RVA
    return RVAToOffset(VAL32(m_pOPTh->DataDirectory[dwDirectory].VirtualAddress),
        pdwSectionOffset, pdwSectionSize);
}

BOOL PEVerifier::CheckCOMHeader() const
{
    DWORD dwSectionOffset, dwSectionSize, dwDirectorySize;
    DWORD dwCOR2Hdr=DirectoryToOffset(IMAGE_DIRECTORY_ENTRY_COMHEADER,&dwDirectorySize,&dwSectionOffset,&dwSectionSize);
    if(dwCOR2Hdr==0)
        return FALSE;
    if (sizeof(IMAGE_COR20_HEADER)>dwDirectorySize)
        return FALSE;

    if ((dwCOR2Hdr + sizeof(IMAGE_COR20_HEADER)) > (dwSectionOffset + dwSectionSize))
        return FALSE;

    IMAGE_COR20_HEADER* pHeader=(IMAGE_COR20_HEADER*)(m_pBase+dwCOR2Hdr);
    if (!(pHeader->VTableFixups.VirtualAddress == 0 && pHeader->VTableFixups.Size == 0))
        return FALSE;
    if(!(pHeader->ExportAddressTableJumps.VirtualAddress == 0 && pHeader->ExportAddressTableJumps.Size == 0))
        return FALSE;

    return TRUE;
}
