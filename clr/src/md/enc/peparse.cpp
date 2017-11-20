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
#include "stdafx.h"
#include <windows.h>
#include <corhdr.h>
#include "corerror.h"
#include "palclr.h"
#include "corimage.h"


STDAPI RuntimeReadHeaders(PBYTE hAddress, IMAGE_DOS_HEADER** ppDos,
                          IMAGE_NT_HEADERS** ppNT, IMAGE_COR20_HEADER** ppCor,
                          BOOL fDataMap, DWORD dwLength);

static const char g_szCORMETA[] = ".cormeta";


EXTERN_C HRESULT FindImageMetaData(PVOID pImage, PVOID *ppMetaData, long *pcbMetaData, DWORD dwFileLength)
{
    HRESULT             hr;
    IMAGE_DOS_HEADER   *pDos;
    IMAGE_COR20_HEADER *pCor;
    
	// This image may have been mapped by the runtime and not had the
	// conversion to PE+ done on 64bit.                                          
    IMAGE_NT_HEADERS   *pNtTemp;

    if (FAILED(hr = RuntimeReadHeaders((PBYTE)pImage, &pDos, &pNtTemp, &pCor, TRUE, dwFileLength)))
        return hr;

	if (pNtTemp->OptionalHeader.Magic == VAL16(IMAGE_NT_OPTIONAL_HDR32_MAGIC))
	{
		IMAGE_NT_HEADERS32   *pNt32 = (IMAGE_NT_HEADERS32*)pNtTemp;
		*ppMetaData = Cor_RtlImageRvaToVa32(pNt32,
		                                  (PBYTE)pImage,
			                              VAL32(pCor->MetaData.VirtualAddress),
				                          dwFileLength);
		*pcbMetaData = VAL32(pCor->MetaData.Size);
	}
	else if (pNtTemp->OptionalHeader.Magic == VAL16(IMAGE_NT_OPTIONAL_HDR64_MAGIC))
	{
		IMAGE_NT_HEADERS64   *pNt64 = (IMAGE_NT_HEADERS64*)pNtTemp;
		*ppMetaData = Cor_RtlImageRvaToVa64(pNt64,
		                                  (PBYTE)pImage,
			                              VAL32(pCor->MetaData.VirtualAddress),
				                          dwFileLength);
		*pcbMetaData = VAL32(pCor->MetaData.Size);
	}
	else
	{
		*ppMetaData = NULL;
		*pcbMetaData = 0;
	}

    if (*ppMetaData == NULL || *pcbMetaData == 0)
        return COR_E_BADIMAGEFORMAT;
    return S_OK;
}


EXTERN_C HRESULT FindObjMetaData(PVOID pImage, PVOID *ppMetaData, ULONG *pcbMetaData, DWORD dwFileLength)
{
    IMAGE_FILE_HEADER *pImageHdr;       // Header for the .obj file.
    IMAGE_SECTION_HEADER *pSectionHdr;  // Section header.
    WORD        i;                      // Loop control.

    // Get a pointer to the header and the first section.
    pImageHdr = (IMAGE_FILE_HEADER *) pImage;
    pSectionHdr = (IMAGE_SECTION_HEADER *)(pImageHdr + 1);

    // Avoid confusion.
    *ppMetaData = NULL;
    *pcbMetaData = 0;

    // Walk each section looking for .cormeta.
    for (i=0;  i<VAL16(pImageHdr->NumberOfSections);  i++, pSectionHdr++)
    {
        // Simple comparison to section name.
        if (strcmp((const char *) pSectionHdr->Name, g_szCORMETA) == 0)
        {
            // Check that raw data in the section is actually within the file.
            if (VAL32(pSectionHdr->PointerToRawData) + VAL32(pSectionHdr->SizeOfRawData) > dwFileLength)
                break;
            *pcbMetaData = VAL32(pSectionHdr->SizeOfRawData);
            *ppMetaData = (PVOID) ((ULONG_PTR) pImage + VAL32(pSectionHdr->PointerToRawData));
            break;
        }
    }

    // Check for errors.
    if (*ppMetaData == NULL || *pcbMetaData == 0)
        return (COR_E_BADIMAGEFORMAT);
    return (S_OK);
}
