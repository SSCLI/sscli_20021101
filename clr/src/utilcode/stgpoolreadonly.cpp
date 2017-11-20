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
// StgPoolReadOnly.cpp
//
// Read only pools are used to reduce the amount of data actually required in the database.
// 
//*****************************************************************************
#include "stdafx.h"                     // Standard include.
#include <stgpool.h>                    // Our interface definitions.
#include "metadatatracker.h"
//
//
// StgPoolReadOnly
//
//


const BYTE StgPoolSeg::m_zeros[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};


//*****************************************************************************
// Free any memory we allocated.
//*****************************************************************************
StgPoolReadOnly::~StgPoolReadOnly()
{
}


//*****************************************************************************
// Init the pool from existing data.
//*****************************************************************************
HRESULT StgPoolReadOnly::InitOnMemReadOnly(// Return code.
        void        *pData,             // Predefined data.
        ULONG       iSize)              // Size of data.
{
    // Make sure we aren't stomping anything and are properly initialized.
    _ASSERTE(m_pSegData == m_zeros);

    // Create case requires no further action.
    if (!pData)
        return (E_INVALIDARG);

    m_pSegData = reinterpret_cast<BYTE*>(pData);
    m_cbSegSize = iSize;
    m_cbSegNext = iSize;
    return (S_OK);
}

//*****************************************************************************
// Prepare to shut down or reinitialize.
//*****************************************************************************
void StgPoolReadOnly::Uninit()
{
	m_pSegData = (BYTE*)m_zeros;
	m_pNextSeg = 0;
}


//*****************************************************************************
// Convert a string to UNICODE into the caller's buffer.
//*****************************************************************************
HRESULT StgPoolReadOnly::GetStringW(      // Return code.
    ULONG       iOffset,                // Offset of string in pool.
    LPWSTR      szOut,                  // Output buffer for string.
    int         cchBuffer)              // Size of output buffer.
{
    LPCSTR      pString;                // The string in UTF8.
    int         iChars;
    pString = GetString(iOffset);
    iChars = ::WszMultiByteToWideChar(CP_UTF8, 0, pString, -1, szOut, cchBuffer);
    if (iChars == 0)
        return (BadError(HRESULT_FROM_NT(GetLastError())));
    return (S_OK);
}


//*****************************************************************************
// Return a pointer to a Guid given an index previously handed out by
// AddGuid or FindGuid.
//*****************************************************************************
GUID *StgPoolReadOnly::GetGuid(			// Pointer to guid in pool.
	ULONG		iIndex)					// 1-based index of Guid in pool.
{
    if (iIndex == 0)
        return (reinterpret_cast<GUID*>(const_cast<BYTE*>(m_zeros)));

	// Convert to 0-based internal form, defer to implementation.
	return (GetGuidi(iIndex-1));
}


//*****************************************************************************
// Return a pointer to a Guid given an index previously handed out by
// AddGuid or FindGuid.
//*****************************************************************************
GUID *StgPoolReadOnly::GetGuidi(		// Pointer to guid in pool.
	ULONG		iIndex)					// 0-based index of Guid in pool.
{
	ULONG iOffset = iIndex * sizeof(GUID);
    _ASSERTE(IsValidOffset(iOffset));
    return (reinterpret_cast<GUID*>(GetData(iOffset)));
}


//*****************************************************************************
// Return a pointer to a null terminated blob given an offset previously
// handed out by Addblob or Findblob.
//*****************************************************************************
void *StgPoolReadOnly::GetBlob(             // Pointer to blob's bytes.
    ULONG       iOffset,                // Offset of blob in pool.
    ULONG       *piSize)                // Return size of blob.
{
    void const  *pData = NULL;          // Pointer to blob's bytes.

    if (iOffset == 0)
    {
        *piSize = 0;
        return (const_cast<BYTE*>(m_zeros));
    }

    // Is the offset within this heap?
    //_ASSERTE(IsValidOffset(iOffset));
	if(!IsValidOffset(iOffset))
	{
		_ASSERTE(!"Invalid Blob Offset");
		iOffset = 0;
	}

    // Get size of the blob (and pointer to data).
    *piSize = CPackedLen::GetLength(GetData(iOffset), &pData);


    // Return pointer to data.
    return (const_cast<void*>(pData));
}



