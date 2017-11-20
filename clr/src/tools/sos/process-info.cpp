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

#ifndef PLATFORM_UNIX

#include "strike.h"
#include "util.h"
#include "process-info.h"
#include <assert.h>
#include "palclr.h"

/**
 * The maxiumum value that we'll consider to be an ordinal.  This may be
 * smaller than the maximum possible, but should suffice for now.
 */
const int MAX_ORDINAL_VALUE = 128;

/**
 * Search for the export by the ordinal
 */
static BOOL _searchExportsByOrdinal (
    DWORD_PTR               BaseOfDll,
    const char*             ExportName,
    IMAGE_EXPORT_DIRECTORY* Table,
    ULONG_PTR*              ExportAddress)
{
    // An ordinal is merely a direct index into the AddressOfFunctions array.
    DWORD ordinal = reinterpret_cast<DWORD>(ExportName);

    assert (ordinal < (DWORD)MAX_ORDINAL_VALUE);

    // Ordinal values are modified by a Base value.
    ordinal -= Table->Base;

    if (ordinal > Table->NumberOfFunctions)
        // invalid ordinal value
        return false;

    ULONG_PTR offsetT;

    //
    // read AddressOfFunctions [ordinal];
    //

    if (!SafeReadMemory ( (ULONG_PTR)(BaseOfDll +
            (ULONG) Table->AddressOfFunctions +
            ordinal * sizeof (DWORD_PTR)),
        &offsetT,
        sizeof (offsetT),
        NULL))
        return FALSE;

    offsetT += BaseOfDll;

    *ExportAddress = offsetT;
    return TRUE;
}

#define MAX_SYMBOL_LENGTH	(260)

//
// This searches for the name in an alphabetical list
//
static BOOL _searchExportsByName (
    DWORD_PTR               BaseOfDll,
    const char*             ExportName,
    IMAGE_EXPORT_DIRECTORY* Table,
    ULONG_PTR*              ExportAddress)
{
    ULONG cbT = sizeof (DWORD) * Table->NumberOfNames;

    PDWORD NamePointerTable = (PDWORD) _alloca(cbT);

    memset (NamePointerTable, 0, cbT);

    if (!SafeReadMemory (
        (ULONG_PTR)((ULONG)Table->AddressOfNames + BaseOfDll),
        NamePointerTable,
        cbT,
        NULL))
        return FALSE;

    //
    // Bsearch the name table
    //
    LONG low = 0;
    LONG high = Table->NumberOfNames - 1;

    while (low <= high)
        {
        CHAR    Buffer [MAX_SYMBOL_LENGTH];
        ULONG i = (low + high) / 2;

        ULONG cbRead;
        if (!SafeReadMemory (
            (ULONG_PTR)(NamePointerTable [i] + BaseOfDll),
            Buffer,
            sizeof(Buffer),
            &cbRead))
            return FALSE;

        LONG iRet = strncmp(Buffer, ExportName, cbRead);

        if (iRet < 0)
            low = i + 1;
        else if (iRet > 0)
            high = i - 1;
        else
            {	// match
            //
            // ordinal = AddressOfNameOrdinals [i];
            //
            ULONG_PTR offsetT = BaseOfDll +
                (ULONG) Table->AddressOfNameOrdinals +
                i * sizeof (USHORT);

            WORD wordT;
            if (!SafeReadMemory ( (ULONG_PTR) offsetT,
                &wordT,
                sizeof (wordT),
                NULL))
                return FALSE;

            DWORD ordinal = wordT;

            //
            // read AddressOfFunctions [ordinal];
            //
            if (!SafeReadMemory ( (ULONG_PTR)(BaseOfDll +
                    (ULONG) Table->AddressOfFunctions +
                    ordinal * sizeof (DWORD)),
                &offsetT,
                sizeof (offsetT),
                NULL))
                return FALSE;

            offsetT += BaseOfDll;

            *ExportAddress = offsetT;
            return TRUE;
            }
        }

    return FALSE;
}


//
// GetExportByName -- look up an exported symbol in a DLL in another
// process. Call this after receiving LOAD_DLL_DEBUG_EVENT.
//
// This code simply reads the export table out of the DLL from the remote
// process and searches it.
//
//

BOOL GetExportByName(
    DWORD_PTR   BaseOfDll,
    const char* ExportName,
    ULONG_PTR*  ExportAddress)
{
    IMAGE_DOS_HEADER        dosHdr;
    IMAGE_NT_HEADERS        ntHdr;
    PIMAGE_DATA_DIRECTORY	  ExportDataDirectory = NULL;
    IMAGE_EXPORT_DIRECTORY  ExportDirectoryTable;
    ULONG                   SectionTableBase;
    ULONG                   NumberOfSections;
    IMAGE_SECTION_HEADER*   rgSecHdr = NULL;

    //
    // Read the DOS header from the front of the image.
    //
    if (!SafeReadMemory ((ULONG_PTR) BaseOfDll,
        &dosHdr,
        sizeof(dosHdr),
        NULL))
        return FALSE;

    //
    // Verify that we've got a valid DOS header.
    //
    if (dosHdr.e_magic != VAL16(IMAGE_DOS_SIGNATURE))
        return FALSE;

    //
    // Read the NT Header next.
    //
    if (!SafeReadMemory ((ULONG_PTR) (VAL32(dosHdr.e_lfanew) + BaseOfDll),
        &ntHdr,
        sizeof(ntHdr),
        NULL))
        return FALSE;

    //
    // Verify that we've got a valid NT Header.
    //
    if (ntHdr.Signature != VAL32(IMAGE_NT_SIGNATURE))
        return FALSE;

    //
    // Grab the section info from the optional headers.
    //
    NumberOfSections = VAL16(ntHdr.FileHeader.NumberOfSections);
    rgSecHdr = (IMAGE_SECTION_HEADER *) _alloca(NumberOfSections *
        sizeof(IMAGE_SECTION_HEADER));

    assert(sizeof(ntHdr.OptionalHeader) ==
        VAL16(ntHdr.FileHeader.SizeOfOptionalHeader));

    SectionTableBase = BaseOfDll + VAL32(dosHdr.e_lfanew) + sizeof(IMAGE_NT_HEADERS);

    if (!SafeReadMemory ((ULONG_PTR) SectionTableBase,
        rgSecHdr,
        NumberOfSections * sizeof (IMAGE_SECTION_HEADER),
        NULL))
        return FALSE;

    ExportDataDirectory =
        &ntHdr.OptionalHeader.DataDirectory [IMAGE_DIRECTORY_ENTRY_EXPORT];

    if (ExportDataDirectory->VirtualAddress == 0 ||
        ExportDataDirectory->Size == 0)
        {
        //
        // No exports.  This happens often -- exes generally do not have
        // exports.
        //
        return FALSE;
        }

    if (!SafeReadMemory ((ULONG_PTR) (VAL32(ExportDataDirectory->VirtualAddress) +
            BaseOfDll),
        &ExportDirectoryTable,
        sizeof (ExportDirectoryTable),
        NULL))
        return FALSE;

    if (ExportDirectoryTable.NumberOfNames == 0L)
        return FALSE;

    // Ordinals are values below a given value.  In this case, we'll assume
    // that ordinals will be values less than MAX_ORDINAL_VALUE
    if (ExportName < reinterpret_cast<const char*>(MAX_ORDINAL_VALUE))
        return _searchExportsByOrdinal (BaseOfDll, ExportName, 
            &ExportDirectoryTable, ExportAddress);

    return _searchExportsByName (BaseOfDll, ExportName, 
        &ExportDirectoryTable, ExportAddress);
}
#endif //!PLATFORM_UNIX

