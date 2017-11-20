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
//  Class:    CultureInfoTable
//  Purpose:  Used to retrieve culture information from culture.nlp                          .
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
#include "cultureinfotable.h"

#define LOCALE_BUFFER_SIZE  32

const CHAR CultureInfoTable::m_lpCultureFileName[]      = "culture.nlp";
const WCHAR CultureInfoTable::m_lpCultureMappingName[]  = L"_nlsplus_culture_1_0_3127_nlp";

CRITICAL_SECTION CultureInfoTable::m_ProtectDefaultTable;
CultureInfoTable * CultureInfoTable::m_pDefaultTable;


/*=================================CultureInfoTable============================
**Action:
**Returns:
**Arguments:
**Exceptions:
==============================================================================*/

CultureInfoTable::CultureInfoTable() :
    BaseInfoTable(SystemDomain::SystemAssembly()) {
    InitializeCriticalSection(&m_ProtectCache);
    InitDataTable(m_lpCultureMappingName, m_lpCultureFileName, m_hBaseHandle);
}

/*=================================~CultureInfoTable============================
**Action:
**Returns:
**Arguments:
**Exceptions:
==============================================================================*/

CultureInfoTable::~CultureInfoTable() {
    DeleteCriticalSection(&m_ProtectCache);
    UninitDataTable();
}

/*==========================InitializeCultureInfoTable==========================
**Action: Intialize critical section variables so they will be only initialized once. 
**        Used by COMNlsInfo::InitializeNLS().
**Returns: None.
**Arguments: None.
**Exceptions: None.
==============================================================================*/

void CultureInfoTable::InitializeTable() {
    InitializeCriticalSection(&m_ProtectDefaultTable);
}

/*===========================ShutdownCultureInfoTable===========================
**Action: Deletes any items that we may have allocated into the CultureInfoTable 
**        cache.                                                                      
**Returns:    Void
**Arguments:  None.  The side effect is to free any allocated memory.
**Exceptions: None.
==============================================================================*/



/*================================AllocateTable=================================
**Action:  This is a very thin wrapper around the constructor. Calls to new can't be
**         made directly in a COMPLUS_TRY block. 
**Returns: A newly allocated CultureInfoTable.
**Arguments: None
**Exceptions: The CultureInfoTable constructor can throw an OutOfMemoryException or
**            an ExecutionEngineException.
==============================================================================*/

CultureInfoTable *CultureInfoTable::AllocateTable() {
    return new CultureInfoTable();
}


/*===============================CreateInstance================================
**Action:  Create the default instance of CultureInfoTable.  This allocates the table if it hasn't
**         previously been allocated.  We need to carefully wrap the call to AllocateTable
**         because the constructor can throw some exceptions.  Unless we have the
**         try/finally block, the exception will skip the LeaveCriticalSection and
**         we'll create a potential deadlock.
**Returns: A pointer to the default CultureInfoTable.
**Arguments: None 
**Exceptions: Can throw an OutOfMemoryException or an ExecutionEngineException.
==============================================================================*/

CultureInfoTable* CultureInfoTable::CreateInstance() {
    THROWSCOMPLUSEXCEPTION();

    if (m_pDefaultTable==NULL) {
        Thread* pThread = GetThread();
        pThread->EnablePreemptiveGC();
        LOCKCOUNTINCL("CreateInstance in CultureInfoTable.cpp");								\
        EnterCriticalSection(&m_ProtectDefaultTable);
        
        pThread->DisablePreemptiveGC();
     
        EE_TRY_FOR_FINALLY {
            //Make sure that nobody allocated the table before us.
            if (m_pDefaultTable==NULL) {
                //Allocate the default table and verify that we got one.
                m_pDefaultTable = AllocateTable();
                if (m_pDefaultTable==NULL) {
                    COMPlusThrowOM();
                }
            }
        } EE_FINALLY {
            //We need to leave the critical section regardless of whether
            //or not we were successful in allocating the table.
            LeaveCriticalSection(&m_ProtectDefaultTable);
            LOCKCOUNTDECL("CreateInstance in CultureInfoTable.cpp");								\

        } EE_END_FINALLY;
    }
    return (m_pDefaultTable);
}

/*=================================GetInstance============================
**Action: Get the default instance of CultureInfoTable.
**Returns: A pointer to the default instance of CultureInfoTable.
**Arguments: None
**Exceptions: None.
**Notes: This method should be called after CreateInstance has been called.
** 
==============================================================================*/

CultureInfoTable *CultureInfoTable::GetInstance() {
    _ASSERTE(m_pDefaultTable);
    return (m_pDefaultTable);
}

int CultureInfoTable::GetDataItem(int cultureID) {
    WORD wPrimaryLang = PRIMARYLANGID(cultureID);

    //
    // Check if the primary language in the parameter is greater than the max number of
    // the primary language.  If yes, this is an invalid culture ID.
    //
    if (wPrimaryLang > m_pHeader->maxPrimaryLang) {
        return (-1);
    }

    WORD wNumSubLang = m_pIDOffsetTable[wPrimaryLang].numSubLang;

    //
    // If the number of sub-languages is zero, it means the primary language ID
    //    is not valid. 
    if (wNumSubLang == 0) {
        return (-1);
    }
    //
    // Search thru the data items and try matching the culture ID.
    //
    int dataItem = m_pIDOffsetTable[wPrimaryLang].dataItemIndex;
    for (int i = 0; i < wNumSubLang; i++)
    {
        if (GetDataItemCultureID(dataItem) == cultureID) {            
            return (dataItem);
        }
        dataItem++;
    }
    return (-1);
}

/*=================================GetDataItemCultureID==================================
**Action: Return the language ID for the specified culture data item index.
**Returns: The culture ID.
**Arguments:
**      dataItem an index to a record in the Culture Data Table.
**Exceptions: None.
==============================================================================*/

int CultureInfoTable::GetDataItemCultureID(int dataItem) {
    return (m_pDataTable[dataItem * m_dataItemSize + CULTURE_ILANGUAGE]);
}

