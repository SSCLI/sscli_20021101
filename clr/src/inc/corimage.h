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
** CorImage.h
**
** IMAGEHLP routines so we can avoid early binding to that DLL.
**
===========================================================*/

#ifndef _CORIMAGE_H_
#define _CORIMAGE_H_

#ifdef  __cplusplus
extern "C" {
#endif

IMAGE_COR20_HEADER *Cor_RtlImageCorHeader(IMAGE_NT_HEADERS *pNtHeaders, VOID *pvBase, ULONG FileLength);

IMAGE_NT_HEADERS *Cor_RtlImageNtHeader(VOID *pvBase);

PIMAGE_SECTION_HEADER Cor_RtlImageRvaToSection(PIMAGE_NT_HEADERS NtHeaders,
                                                        ULONG Rva,
                                                        ULONG FileLength);

PIMAGE_SECTION_HEADER Cor_RtlImageRvaRangeToSection(PIMAGE_NT_HEADERS NtHeaders,
                                                             ULONG Rva, ULONG Range,
                                                             ULONG FileLength);

DWORD Cor_RtlImageRvaToOffset(PIMAGE_NT_HEADERS NtHeaders,
                                       ULONG Rva,
                                       ULONG FileLength);

PBYTE Cor_RtlImageRvaToVa(PIMAGE_NT_HEADERS NtHeaders,
                                   PBYTE Base,
                                   ULONG Rva,
                                   ULONG FileLength);

PBYTE Cor_RtlImageRvaToVa32(PIMAGE_NT_HEADERS32 NtHeaders,
                                   PBYTE Base,
                                   ULONG Rva,
                                   ULONG FileLength);

PBYTE Cor_RtlImageRvaToVa64(PIMAGE_NT_HEADERS64 NtHeaders,
                                   PBYTE Base,
                                   ULONG Rva,
                                   ULONG FileLength);

#ifdef __cplusplus
}
#endif

#endif // _CORIMAGE_H_
