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
////////////////////////////////////////////////////////////////////////////
//
//  Class:    BaseInfoTable
//  Purpose:  Base class for CultureInfoTable and RegionInfoTable
//
//
//  Date:     01/21/2000
//
////////////////////////////////////////////////////////////////////////////


#include "common.h"

#include <winnls.h>

#include "comstring.h"
#include "winwrap.h"

#include "comnlsinfo.h"
#include "nlstable.h"
#include "baseinfotable.h"


BaseInfoTable::BaseInfoTable(Assembly* pAssembly) :
    NLSTable(pAssembly) {
}

/**
**        The index is the beginning of the first sub-languages for this primary 
**        languages, followed by the second sub-languages, the third sub-languages, etc.
**        From this index, we can get the data table index for a valid
**        culture ID.
*/

/*=================================InitDataTable============================
**Action: Read data table and initialize pointers to the different parts
**        of the table.
**Returns: void.
**Arguments:
**        lpwMappingName
**        lpFileName
**        hHandle
**Exceptions:
==============================================================================*/

void BaseInfoTable::InitDataTable(LPCWSTR lpwMappingName, LPCSTR lpFileName, HANDLE& hHandle ) {
    LPBYTE pBytePtr = (LPBYTE)MapDataFile(lpwMappingName, lpFileName, &hHandle);

    // Set up pointers to different parts of the table.
    m_pBasePtr = (LPWORD)pBytePtr;
    m_pHeader  = (CultureInfoHeader*)m_pBasePtr;
    m_pWordRegistryTable    = (LPWORD)(pBytePtr + m_pHeader->wordRegistryOffset);
    m_pStringRegistryTable  = (LPWORD)(pBytePtr + m_pHeader->stringRegistryOffset);
    m_pIDOffsetTable    = (IDOffsetItem*)(pBytePtr + m_pHeader->IDTableOffset);
    m_pNameOffsetTable  = (NameOffsetItem*)(pBytePtr + m_pHeader->nameTableOffset);
    m_pDataTable        = (LPWORD)(pBytePtr + m_pHeader->dataTableOffset);
    m_pStringPool       = (LPWSTR)(pBytePtr + m_pHeader->stringPoolOffset);

    m_dataItemSize = m_pHeader->numWordFields + m_pHeader->numStrFields;
}

/*=================================UninitDataTable============================
**Action: Release used resources.
**Returns: Void
**Arguments: Void
**Exceptions: None.
==============================================================================*/

void BaseInfoTable::UninitDataTable()
{
    UnmapViewOfFile((LPCVOID)m_pBasePtr);
    CloseHandle(m_hBaseHandle);
}

/*=================================GetDataItem==================================
**Action: Given a culture ID, return the index which points to
**        the corresponding record in Culture Data Table.
**Returns: an int index points to a record in Culture Data Table.  If no corresponding
**         index to return (because the culture ID is invalid), -1 is returned.
**Arguments:
**         cultureID the specified culture ID.
**Exceptions: None.
==============================================================================*/

int BaseInfoTable::GetDataItem(int cultureID) {
    WORD wPrimaryLang = PRIMARYLANGID(cultureID);
    WORD wSubLang    = SUBLANGID(cultureID);

    //
    // Check if the primary language in the parameter is greater than the max number of
    // the primary language.  If yes, this is an invalid culture ID.
    //
    if (wPrimaryLang > m_pHeader->maxPrimaryLang) {
        return (-1);
    }

    WORD wNumSubLang = m_pIDOffsetTable[wPrimaryLang].numSubLang;

    // Check the following:
    // 1. If the number of sub-languages is zero, it means the primary language ID
    //    is not valid. 
    // 2. Check if the sub-language is in valid range.    
    if (wNumSubLang == 0 || (wSubLang >= wNumSubLang)) {
        return (-1);
    }
    return (m_pIDOffsetTable[wPrimaryLang].dataItemIndex + wSubLang);
}


/*=================================GetInt32Data==================================
**Action: Get the data in the specified data item.  The type of the field is INT32.
**                                                                                        
**
**Returns: The requested INT32 value.
**Arguments:
**      dataItem an index to a record in the Culture Data Table.
**      field a field in the record.  See CultureInfoTable.h for list of fields.
**Exceptions: None.  Caller should make sure dataItem and field are valid.
==============================================================================*/

INT32 BaseInfoTable::GetInt32Data(int dataItem, int field, BOOL useUserOverride) {
    _ASSERTE(dataItem < m_pHeader->numCultures);
    _ASSERTE(field < m_pHeader->numWordFields);


    return (m_pDataTable[dataItem * m_dataItemSize + field]);
}

/*=================================GetDefaultInt32Data==================================
**Action: Get the data in the specified data item.  The type of the field is INT32.
**
**Returns: The requested INT32 value.
**Arguments:
**      dataItem an index to a record in the Culture Data Table.
**      field a field in the record.  See CultureInfoTable.h for list of fields.
**Exceptions: None.  Caller should make sure dataItem and field are valid.
==============================================================================*/

INT32 BaseInfoTable::GetDefaultInt32Data(int dataItem, int field) {
    _ASSERTE(dataItem < m_pHeader->numCultures);
    _ASSERTE(field < m_pHeader->numWordFields);
    return (m_pDataTable[dataItem * m_dataItemSize + field]);
}


/*=================================GetStringData==================================
**Action: Get the data in the specified data item.  Type of the field is LPWSTR.
**                                                                                        
**
**Returns: The requested LPWSTR value.
**      dataItem an index to a record in the Culture Data Table.
**      field a field in the record.  See CultureInfoTable.h for list of fields.
**Exceptions: None.  Caller should make sure dataItem and field are valid.
==============================================================================*/

LPWSTR BaseInfoTable::GetStringData(int dataItem, int field, BOOL useUserOverride, LPWSTR buffer, int bufferSize) {
    _ASSERTE(dataItem < m_pHeader->numCultures);
    _ASSERTE(field < m_pHeader->numStrFields);

    
    //                           use the default data from the data table.
    return (m_pStringPool+m_pDataTable[dataItem * m_dataItemSize + m_pHeader->numWordFields + field]);
}

/*=================================GetDefaultStringData==================================
**Action: Get the data in the specified data item.  Type of the field is LPWSTR.
**
**Returns: The requested LPWSTR value.
**      dataItem an index to a record in the Culture Data Table.
**      field a field in the record.  See CultureInfoTable.h for list of fields.
**Exceptions: None.  Caller should make sure dataItem and field are valid.
==============================================================================*/

LPWSTR BaseInfoTable::GetDefaultStringData(int dataItem, int field) {
    _ASSERTE(dataItem < m_pHeader->numCultures);
    _ASSERTE(field < m_pHeader->numStrFields);
    return (m_pStringPool+m_pDataTable[dataItem * m_dataItemSize + m_pHeader->numWordFields + field]);
}

/*===============================GetHeader==============================
**Action: Return the header structure.
**Returns:
**Arguments:
**Exceptions:
==============================================================================*/

CultureInfoHeader*  BaseInfoTable::GetHeader()
{
    return (m_pHeader);
}

/*=================================GetNameOffsetTable============================
**Action: Return the pointer to the Culture Name Offset Table.
**Returns:
**Arguments:
**Exceptions:
==============================================================================*/

NameOffsetItem* BaseInfoTable::GetNameOffsetTable() {
    return (m_pNameOffsetTable);
}

/*=================================GetStringPool============================
**Action: Return the pointer to the String Pool Table.
**Returns:
**Arguments:
**Exceptions:
==============================================================================*/

LPWSTR BaseInfoTable::GetStringPoolTable() {
    return (m_pStringPool);
}

