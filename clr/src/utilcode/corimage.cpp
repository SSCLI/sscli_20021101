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

/*============================================================
**
** CorImage.cpp
**
** IMAGEHLP routines so we can avoid early binding to that DLL.
**
===========================================================*/

#include "stdafx.h"

#include "corimage.h"
#include "corhdr.h"

#define RTL_MEG                   (1024UL * 1024UL)
#define RTLP_IMAGE_MAX_DOS_HEADER ( 256UL * RTL_MEG)

IMAGE_COR20_HEADER *Cor_RtlImageCorHeader(IMAGE_NT_HEADERS *pNtHeader,
                                          VOID *pvBase,
                                          ULONG FileLength)
{
    return (IMAGE_COR20_HEADER *) Cor_RtlImageRvaToVa(pNtHeader,
                                                      (BYTE *)pvBase,
                                                      VAL32(pNtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COMHEADER].VirtualAddress),
                                                      FileLength);
}

IMAGE_NT_HEADERS *Cor_RtlImageNtHeader(VOID *pvBase)
{
    IMAGE_NT_HEADERS *pNtHeaders = NULL;
    IMAGE_DOS_HEADER *pDos = (IMAGE_DOS_HEADER*)pvBase;
    if (pDos != NULL && pDos != (VOID*)-1) {
        PAL_TRY {
            if ((pDos->e_magic == VAL16(IMAGE_DOS_SIGNATURE)) &&
                ((DWORD)VAL32(pDos->e_lfanew) < RTLP_IMAGE_MAX_DOS_HEADER)) {
                pNtHeaders = (IMAGE_NT_HEADERS*)((BYTE*)pDos + VAL32(pDos->e_lfanew));
                if (pNtHeaders->Signature != VAL32(IMAGE_NT_SIGNATURE))
                    pNtHeaders = NULL;
            }
        }
        PAL_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
            pNtHeaders = NULL;
        }
        PAL_ENDTRY
    }
    return pNtHeaders;
}

EXTERN_C PIMAGE_SECTION_HEADER Cor_RtlImageRvaToSection32(PIMAGE_NT_HEADERS32 NtHeaders,
                                                        ULONG Rva,
                                                        ULONG FileLength)
{
    ULONG i;
    PIMAGE_SECTION_HEADER NtSection;

    NtSection = IMAGE_FIRST_SECTION( NtHeaders );
    for (i=0; i<VAL16(NtHeaders->FileHeader.NumberOfSections); i++) {
        if (FileLength && VAL32(NtSection->PointerToRawData) + VAL32(NtSection->SizeOfRawData) > FileLength)
            return NULL;
        if (Rva >= VAL32(NtSection->VirtualAddress) &&
            Rva < VAL32(NtSection->VirtualAddress) + VAL32(NtSection->SizeOfRawData))
            return NtSection;
        
        ++NtSection;
    }

    return NULL;
}

EXTERN_C PIMAGE_SECTION_HEADER Cor_RtlImageRvaToSection64(PIMAGE_NT_HEADERS64 NtHeaders,
                                                        ULONG Rva,
                                                        ULONG FileLength)
{
    ULONG i;
    PIMAGE_SECTION_HEADER NtSection;

    NtSection = IMAGE_FIRST_SECTION( NtHeaders );
    for (i=0; i<VAL16(NtHeaders->FileHeader.NumberOfSections); i++) {
        if (FileLength && VAL32(NtSection->PointerToRawData) + VAL32(NtSection->SizeOfRawData) > FileLength)
            return NULL;
        if (Rva >= VAL32(NtSection->VirtualAddress) &&
            Rva < VAL32(NtSection->VirtualAddress) + VAL32(NtSection->SizeOfRawData))
            return NtSection;
        
        ++NtSection;
    }

    return NULL;
}

EXTERN_C PIMAGE_SECTION_HEADER Cor_RtlImageRvaToSection(PIMAGE_NT_HEADERS NtHeaders,
                                                        ULONG Rva,
                                                        ULONG FileLength)
{
	if (NtHeaders->OptionalHeader.Magic == VAL16(IMAGE_NT_OPTIONAL_HDR32_MAGIC))
		return Cor_RtlImageRvaToSection32((IMAGE_NT_HEADERS32*)NtHeaders,Rva,FileLength);
	else if(NtHeaders->OptionalHeader.Magic == VAL16(IMAGE_NT_OPTIONAL_HDR64_MAGIC))
		return Cor_RtlImageRvaToSection64((IMAGE_NT_HEADERS64*)NtHeaders,Rva,FileLength);
	else
	{
		_ASSERTE(!"Invalid File Type");
		return NULL;
	}

}

EXTERN_C PBYTE Cor_RtlImageRvaToVa32(PIMAGE_NT_HEADERS32 NtHeaders,
                                     PBYTE Base,
                                     ULONG Rva,
                                     ULONG FileLength)
{
    PIMAGE_SECTION_HEADER NtSection = Cor_RtlImageRvaToSection32(NtHeaders,
                                                                 Rva,
                                                                 FileLength);

    if (NtSection)
        return (Base +
                (Rva - VAL32(NtSection->VirtualAddress)) +
                VAL32(NtSection->PointerToRawData));
    else
        return NULL;
}

EXTERN_C PBYTE Cor_RtlImageRvaToVa64(PIMAGE_NT_HEADERS64 NtHeaders,
                                   PBYTE Base,
                                   ULONG Rva,
                                   ULONG FileLength)
{
    PIMAGE_SECTION_HEADER NtSection = Cor_RtlImageRvaToSection64(NtHeaders,
                                                               Rva,
                                                               FileLength);

    if (NtSection)
        return (Base +
                (Rva - VAL32(NtSection->VirtualAddress)) +
                VAL32(NtSection->PointerToRawData));
    else
        return NULL;
}

EXTERN_C PBYTE Cor_RtlImageRvaToVa(PIMAGE_NT_HEADERS NtHeaders,
                                   PBYTE Base,
                                   ULONG Rva,
                                   ULONG FileLength)
{
	if (NtHeaders->OptionalHeader.Magic == VAL16(IMAGE_NT_OPTIONAL_HDR32_MAGIC))
		return Cor_RtlImageRvaToVa32((IMAGE_NT_HEADERS32*)NtHeaders,Base,Rva,FileLength);
	else if(NtHeaders->OptionalHeader.Magic == VAL16(IMAGE_NT_OPTIONAL_HDR64_MAGIC))
		return Cor_RtlImageRvaToVa64((IMAGE_NT_HEADERS64*)NtHeaders,Base,Rva,FileLength);
	else
	{
		_ASSERTE(!"Invalid File Type");
		return NULL;
	}
}

EXTERN_C PIMAGE_SECTION_HEADER Cor_RtlImageRvaRangeToSection(PIMAGE_NT_HEADERS NtHeaders,
                                                             ULONG Rva, ULONG Range,
                                                             ULONG FileLength)
{
    ULONG i;
    PIMAGE_SECTION_HEADER NtSection;

    if (!Range)
    {
        return Cor_RtlImageRvaToSection(NtHeaders, Rva, FileLength);
    }

    NtSection = IMAGE_FIRST_SECTION( NtHeaders );
    for (i=0; i<VAL16(NtHeaders->FileHeader.NumberOfSections); i++) {
        if (FileLength && VAL32(NtSection->PointerToRawData) + VAL32(NtSection->SizeOfRawData) > FileLength)
            return NULL;
        if (Rva >= VAL32(NtSection->VirtualAddress) &&
            Rva + Range <= VAL32(NtSection->VirtualAddress) + VAL32(NtSection->SizeOfRawData))
            return NtSection;
        
        ++NtSection;
    }

    return NULL;
}

EXTERN_C DWORD Cor_RtlImageRvaToOffset(PIMAGE_NT_HEADERS NtHeaders,
                                       ULONG Rva,
                                       ULONG FileLength)
{
    PIMAGE_SECTION_HEADER NtSection = Cor_RtlImageRvaToSection(NtHeaders,
                                                               Rva,
                                                               FileLength);

    if (NtSection)
        return ((Rva - VAL32(NtSection->VirtualAddress)) +
                VAL32(NtSection->PointerToRawData));
    else
        return NULL;
}
