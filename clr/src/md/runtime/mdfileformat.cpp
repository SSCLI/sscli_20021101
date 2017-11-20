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
// MDFileFormat.cpp
//
// This file contains a set of helpers to verify and read the file format.
// This code does not handle the paging of the data, or different types of
// I/O.  See the StgTiggerStorage and StgIO code for this level of support.
//
//*****************************************************************************
#include "stdafx.h"                     // Standard header file.
#include "mdfileformat.h"               // The format helpers.
#include "posterror.h"                  // Error handling code.


//*****************************************************************************
// Verify the signature at the front of the file to see what type it is.
//*****************************************************************************
#define STORAGE_MAGIC_OLD_SIG   0x2B4D4F43  // BSJB
HRESULT MDFormat::VerifySignature(
    STORAGESIGNATURE *pSig)             // The signature to check.
{
    HRESULT     hr = S_OK;

    // If signature didn't match, you shouldn't be here.
	if (pSig->Signature() == STORAGE_MAGIC_OLD_SIG)
        return (PostError(CLDB_E_FILE_OLDVER, 1, 0));
    if (pSig->Signature() != STORAGE_MAGIC_SIG)
        return (PostError(CLDB_E_FILE_CORRUPT));

    // There is currently no code to migrate an old format of the 1.x.  This
    // would be added only under special circumstances.
    else if (pSig->MajorVer() != FILE_VER_MAJOR || pSig->MinorVer() != FILE_VER_MINOR)
        hr = CLDB_E_FILE_OLDVER;

    if (FAILED(hr))
        hr = PostError(hr, (int) pSig->MajorVer(), (int) pSig->MinorVer());
    return (hr);
}

//*****************************************************************************
// Skip over the header and find the actual stream data.
//*****************************************************************************
STORAGESTREAM *MDFormat::GetFirstStream(// Return pointer to the first stream.
    STORAGEHEADER *pHeader,             // Return copy of header struct.
    const void *pvMd)                   // Pointer to the full file.
{
    const BYTE  *pbMd;              // Working pointer.

    // You need to call this before parsing the file.
    _ASSERTE(VerifySignature((STORAGESIGNATURE *) pvMd) == S_OK);

    // Header data starts after signature.
    pbMd = (const BYTE *) pvMd;
    pbMd += sizeof(STORAGESIGNATURE);
    pbMd += ((STORAGESIGNATURE*)pvMd)->VersionStringLength();
    STORAGEHEADER *pHdr = (STORAGEHEADER *) pbMd;
    *pHeader = *pHdr;
    pbMd += sizeof(STORAGEHEADER);

    // If there is extra data, skip over it.
    if (pHdr->fFlags & STGHDR_EXTRADATA)
        pbMd = pbMd + sizeof(ULONG) + VAL32(*(ULONG *) pbMd);

    // The pointer is now at the first stream in the list.
    return ((STORAGESTREAM *) pbMd);
}
