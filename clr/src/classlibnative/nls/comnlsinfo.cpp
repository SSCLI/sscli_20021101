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
//  Class:    COMNlsInfo
//  Purpose:  This module implements the methods of the COMNlsInfo
//            class.  These methods are the helper functions for the
//            Locale class.
//
//  Date:     August 12, 1998
//
////////////////////////////////////////////////////////////////////////////


//
//  Include Files.
//

#include "common.h"
#include "object.h"
#include "excep.h"
#include "vars.hpp"
#include "comstring.h"
#include "interoputil.h"

#include <winnls.h>

#include "utilcode.h"
#include "frames.h"
#include "field.h"
#include "metasig.h"
#include "comnls.h"
#include "gcscan.h"
#include "comnlsinfo.h"
#include "nlstable.h"
#include "nativetextinfo.h"
#include "casingtable.h"        // class CasingTable
#include "globalizationassembly.h"
#include "sortingtablefile.h"
#include "sortingtable.h"
#include "baseinfotable.h"
#include "cultureinfotable.h"
#include "regioninfotable.h"
#include "calendartable.h"
#include "wsperf.h"


#include "unicodecattable.h"


//
//  Constant Declarations.
//
#ifndef COMPARE_OPTIONS_ORDINAL
#define COMPARE_OPTIONS_ORDINAL            0x40000000
#endif

#ifndef COMPARE_OPTIONS_IGNORECASE
#define COMPARE_OPTIONS_IGNORECASE            0x00000001
#endif

#define NLS_CP_MBTOWC             0x40000000                    
#define NLS_CP_WCTOMB             0x80000000                    

#define MAX_STRING_VALUE        512

CasingTable* COMNlsInfo::m_pCasingTable = NULL;


//
// 
BOOL COMNlsInfo::InitializeNLS() {
    CultureInfoTable::InitializeTable();
    RegionInfoTable::InitializeTable();
    CalendarTable::InitializeTable();
    return TRUE; //Made a boolean in case we have further initialization in the future.
}


/*============================nativeCreateGlobalizationAssembly============================
**Action: Create NativeGlobalizationAssembly instance for the specified Assembly.
**Returns: 
**  void.  
**  The side effect is to allocate the NativeCompareInfo cache.
**Arguments:  None
**Exceptions: OutOfMemoryException if we run out of memory.
** 
**NOTE NOTE: This is a synchronized operation.  The required synchronization is
**           provided by the fact that we only call this in the class initializer
**           for CompareInfo.  If this invariant ever changes, guarantee 
**           synchronization.
==============================================================================*/

FCIMPL1(LPVOID, COMNlsInfo::nativeCreateGlobalizationAssembly, AssemblyBaseObject* pAssemblyRef) 
{
    NativeGlobalizationAssembly* pNGA;

    Assembly *pAssembly = pAssemblyRef->GetAssembly();

    HELPER_METHOD_FRAME_BEGIN_RET_0();
    THROWSCOMPLUSEXCEPTION();

    if ((pNGA = NativeGlobalizationAssembly::FindGlobalizationAssembly(pAssembly))==NULL) {
        // Get the native pointer to Assembly from the ASSEMBLYREF, and use the pointer
        // to construct NativeGlobalizationAssembly.
        pNGA = new NativeGlobalizationAssembly(pAssembly);
        if (pNGA == NULL) {
            COMPlusThrowOM();
        }
        
        // Always add the newly created NGA to the static linked list of NativeGlobalizationAssembly.
        // This step is necessary so that we can shut down the SortingTable correctly.
        NativeGlobalizationAssembly::AddToList(pNGA);
    }
    HELPER_METHOD_FRAME_END();

    RETURN(pNGA, LPVOID);
}
FCIMPLEND

/*=============================InitializeNativeCompareInfo==============================
**Action: A very thin wrapper on top of the NativeCompareInfo class that prevents us
**        from having to include SortingTable.h in ecall.
**Returns: The LPVOID pointer to the constructed NativeCompareInfo for the specified sort ID.
**        The side effect is to allocate a particular sorting table
**Arguments:
**        pAssembly the NativeGlobalizationAssembly instance used to load the sorting data tables.
**        sortID    the sort ID.
**Exceptions: OutOfMemoryException if we run out of memory.
**            ExecutionEngineException if the needed resources cannot be loaded.
** 
**NOTE NOTE: This is a synchronized operation.  The required synchronization is
**           provided by making CompareInfo.InitializeSortTable a sychronized
**           operation.  If you call this method from anyplace else, ensure 
**           that synchronization remains intact.
==============================================================================*/
    
FCIMPL2(LPVOID, COMNlsInfo::InitializeNativeCompareInfo, INT_PTR pNativeGlobalizationAssembly, INT32 sortID) 
{
    THROWSCOMPLUSEXCEPTION();

    // Ask the SortingTable instance in pNativeGlobalizationAssembly to get back the 
    // NativeCompareInfo object for the specified LCID.
    NativeCompareInfo*              pNativeCompareInfo;
    NativeGlobalizationAssembly*    pNGA = (NativeGlobalizationAssembly*)pNativeGlobalizationAssembly;

    HELPER_METHOD_FRAME_BEGIN_RET_0();
    pNativeCompareInfo = pNGA->m_pSortingTable->InitializeNativeCompareInfo(sortID);
    HELPER_METHOD_FRAME_END();

    if (pNativeCompareInfo == NULL) {
        FCThrow(kOutOfMemoryException);
    }

    RETURN(pNativeCompareInfo, LPVOID);
}
FCIMPLEND



////////////////////////////////////////////////////////////////////////////
//
//  IsSupportedLCID
//
////////////////////////////////////////////////////////////////////////////

FCIMPL1(INT32, COMNlsInfo::IsSupportedLCID, INT32 lcid) {
    return (::IsValidLocale(lcid, LCID_SUPPORTED));
}
FCIMPLEND


FCIMPL1(INT32, COMNlsInfo::IsInstalledLCID, INT32 lcid) {
    BOOL bResult = ::IsValidLocale(lcid, LCID_INSTALLED);
    return (bResult);
}
FCIMPLEND


////////////////////////////////////////////////////////////////////////////
//
//  nativeGetUserDefaultLCID
//
////////////////////////////////////////////////////////////////////////////

FCIMPL0(INT32, COMNlsInfo::nativeGetUserDefaultLCID) {
    return (::GetUserDefaultLCID());
}
FCIMPLEND

/*       
 */
FCIMPL0(INT32, COMNlsInfo::nativeGetUserDefaultUILanguage)
{
    THROWSCOMPLUSEXCEPTION();
    LANGID uiLangID = 0;

    uiLangID = GetUserDefaultLangID();
    // Return the found language ID.
    return (uiLangID);
}
FCIMPLEND



//    
 
FCIMPL0(INT32, COMNlsInfo::nativeGetSystemDefaultUILanguage)
{
    THROWSCOMPLUSEXCEPTION();

    LANGID uiLangID = 0;
    HELPER_METHOD_FRAME_BEGIN_RET_0();


        uiLangID = ::GetSystemDefaultLangID();

    HELPER_METHOD_FRAME_END();
    
    return (uiLangID);
}
FCIMPLEND


/*=================================nativeInitCultureInfoTable============================
**Action: Create the default instance of CultureInfoTable.
**Returns: void.
**Arguments: void.
**Exceptions:
**      OutOfMemoryException if the creation fails.
==============================================================================*/

FCIMPL0(VOID, COMNlsInfo::nativeInitCultureInfoTable) {
    HELPER_METHOD_FRAME_BEGIN_0();
    CultureInfoTable::CreateInstance();
    HELPER_METHOD_FRAME_END();
}
FCIMPLEND

/*==========================GetCultureInfoStringPoolTable======================
**Action: Return a pointer to the String Pool Table string in CultureInfoTable.
**        No range checking of any sort is performed.
**Returns:
**Arguments:
**Exceptions:
==============================================================================*/

FCIMPL1(LPWSTR, COMNlsInfo::nativeGetCultureInfoStringPoolStr, INT32 offset) {
    _ASSERTE(CultureInfoTable::GetInstance());
    return (CultureInfoTable::GetInstance()->GetStringPoolTable() + offset);
}
FCIMPLEND

/*=========================nativeGetCultureInfoHeader======================
**Action: Return a pointer to the header in
**        CultureInfoTable.
**Returns:
**Arguments:
**Exceptions:
==============================================================================*/

FCIMPL0(CultureInfoHeader*, COMNlsInfo::nativeGetCultureInfoHeader) {
    _ASSERTE(CultureInfoTable::GetInstance());
    return (CultureInfoTable::GetInstance()->GetHeader());
}
FCIMPLEND

/*=========================GetCultureInfoNameOffsetTable======================
**Action: Return a pointer to an item in the Culture Name Offset Table in
**        CultureInfoTable.
**Returns:
**Arguments:
**Exceptions:
==============================================================================*/

FCIMPL0(NameOffsetItem*, COMNlsInfo::nativeGetCultureInfoNameOffsetTable) {
    _ASSERTE(CultureInfoTable::GetInstance());
    return (CultureInfoTable::GetInstance()->GetNameOffsetTable());
}
FCIMPLEND

/*=======================nativeGetCultureDataFromID=============================
**Action: Given a culture ID, return the index which points to
**        the corresponding record in Culture Data Table.  The index is referred
**        as 'Culture Data Item Index' in the code.
**Returns: an int index points to a record in Culture Data Table.  If no corresponding
**         index to return (because the culture ID is valid), -1 is returned.
**Arguments:
**         culture ID the specified culture ID.
**Exceptions: None.
==============================================================================*/

FCIMPL1(INT32,COMNlsInfo::nativeGetCultureDataFromID, INT32 nCultureID) {
    return (CultureInfoTable::GetInstance()->GetDataItem(nCultureID));
}
FCIMPLEND

/*=============================GetCultureInt32Value========================
**Action: Return the WORD value for a specific information of a culture.
**        This is used to query values like 'the number of decimal digits' for
**        a culture.
**Returns: An int for the required value (However, the value is always in WORD range).
**Arguments:
**      nCultureDataItem    the Culture Data Item index.  This is an index 
**                          which points to the corresponding record in the
**                          Culture Data Table.
**      nValueField  an integer indicating which fields that we are interested.
**                  See CultureInfoTable.h for a list of the fields.
**Exceptions:
==============================================================================*/

FCIMPL3(INT32, COMNlsInfo::GetCultureInt32Value, INT32 CultureDataItem, INT32 ValueField, BYTE UseUserOverride) {
    INT32 retVal = 0;

    retVal = CultureInfoTable::GetInstance()->GetInt32Data(CultureDataItem, ValueField, UseUserOverride);
    return (retVal);
}
FCIMPLEND

FCIMPL2(INT32, COMNlsInfo::GetCultureDefaultInt32Value, INT32 CultureDataItem, INT32 ValueField) {
    INT32 retVal = 0;

    retVal = CultureInfoTable::GetInstance()->GetDefaultInt32Data(CultureDataItem, ValueField); 
    return (retVal);
}
FCIMPLEND


FCIMPL3(LPVOID, COMNlsInfo::GetCultureStringValue, INT32 CultureDataItem, INT32 ValueField, BYTE UseUserOverride)
{
    STRINGREF  refRetString = NULL;

    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, refRetString);

    WCHAR InfoStr[MAX_STRING_VALUE];

    LPWSTR pStringValue = CultureInfoTable::GetInstance()->GetStringData(CultureDataItem, ValueField, UseUserOverride, InfoStr, MAX_STRING_VALUE);
    refRetString = COMString::NewString(pStringValue);

    HELPER_METHOD_FRAME_END();

    RETURN(refRetString, STRINGREF);
}
FCIMPLEND

FCIMPL2(LPVOID, COMNlsInfo::GetCultureDefaultStringValue, INT32 CultureDataItem, INT32 ValueField) 
{
    LPWSTR pInfoStr = CultureInfoTable::GetInstance()->GetDefaultStringData(CultureDataItem, ValueField);

    StringObject* pString = NULL;

    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, pString);

    pString = (StringObject*)(OBJECTREFToObject(COMString::NewString(pInfoStr)));

    HELPER_METHOD_FRAME_END();

    FC_GC_POLL_AND_RETURN_OBJREF(pString);
}
FCIMPLEND


/*=================================GetMultiStringValues==========================
**Action:
**Returns:
**Arguments:
**Exceptions:
============================================================================*/

PTRARRAYREF COMNlsInfo::GetMultiStringValues(LPWSTR pInfoStr) 
{
    THROWSCOMPLUSEXCEPTION();

    //
    // Get the first string.
    //
    if (pInfoStr == NULL) {
        return (NULL);
    }

    //
    // Create a dynamic array to store multiple strings.
    //
    CUnorderedArray<WCHAR *, CULTUREINFO_OPTIONS_SIZE> * pStringArray;
    pStringArray = new CUnorderedArray<WCHAR *, CULTUREINFO_OPTIONS_SIZE>();
    
    if (!pStringArray) {
        COMPlusThrowOM();
    }

    //
    // We can't store STRINGREFs in an unordered array because the GC won't track
    // them properly.  To work around this, we'll count the number of strings
    // which we need to allocate and store a wchar* for the beginning of each string.
    // In the loop below, we'll walk this array of wchar*'s and allocate a managed
    // string for each one.
    //
    while (*pInfoStr != NULL) {
        *(pStringArray->Append()) = pInfoStr;
        //
        // Advance to next string.
        //
        pInfoStr += (Wszlstrlen(pInfoStr) + 1);
    }


    //
    // Allocate the array of STRINGREFs.  We don't need to check for null because the GC will throw 
    // an OutOfMemoryException if there's not enough memory.
    //
    PTRARRAYREF ResultArray = (PTRARRAYREF)AllocateObjectArray(pStringArray->Count(), g_pStringClass);

//    LPVOID lpvReturn;
    STRINGREF pString;
    INT32 stringCount = pStringArray->Count();

    //
    // Walk the wchar*'s and allocate a string for each one which we put into the result array.
    //
    GCPROTECT_BEGIN(ResultArray);    
    for (int i = 0; i < stringCount; i++) {
        pString = COMString::NewString(pStringArray->m_pTable[i]);    
        ResultArray->SetAt(i, (OBJECTREF)pString);
    }
//    *((PTRARRAYREF *)(&lpvReturn))=ResultArray;
    GCPROTECT_END();

    delete (pStringArray);

    return (ResultArray);    
}

FCIMPL3(Object*, COMNlsInfo::GetCultureMultiStringValues, INT32 CultureDataItem, INT32 ValueField, BYTE UseUserOverride)
{
    PTRARRAYREF refRetVal = NULL;
    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, refRetVal);

    WCHAR InfoStr[MAX_STRING_VALUE];
    LPWSTR pMultiStringValue = CultureInfoTable::GetInstance()->GetStringData(
        CultureDataItem, ValueField, UseUserOverride, InfoStr, MAX_STRING_VALUE);

    refRetVal = GetMultiStringValues(pMultiStringValue);

    HELPER_METHOD_FRAME_END();
    return OBJECTREFToObject(refRetVal);
}
FCIMPLEND

/*=================================nativeInitRegionInfoTable============================
**Action: Create the default instance of RegionInfoTable.
**Returns: void.
**Arguments: void.
**Exceptions:
**      OutOfMemoryException if the creation fails.
==============================================================================*/

FCIMPL0(VOID, COMNlsInfo::nativeInitRegionInfoTable)
{
    HELPER_METHOD_FRAME_BEGIN_0();
    RegionInfoTable::CreateInstance();
    HELPER_METHOD_FRAME_END();
}
FCIMPLEND

/*==========================GetRegionInfoStringPoolTable======================
**Action: Return a pointer to the String Pool Table string in RegionInfoTable.
**        No range checking of any sort is performed.
**Returns:
**Arguments:
**Exceptions:
==============================================================================*/

FCIMPL1(LPWSTR, COMNlsInfo::nativeGetRegionInfoStringPoolStr, INT32 offset) 
{
    _ASSERTE(RegionInfoTable::GetInstance());
    return (RegionInfoTable::GetInstance()->GetStringPoolTable() + offset);
}
FCIMPLEND

/*=========================nativeGetRegionInfoHeader======================
**Action: Return a pointer to the header in
**        RegionInfoTable.
**Returns:
**Arguments:
**Exceptions:
==============================================================================*/

FCIMPL0(CultureInfoHeader*, COMNlsInfo::nativeGetRegionInfoHeader) {
    _ASSERTE(RegionInfoTable::GetInstance());
    return (RegionInfoTable::GetInstance()->GetHeader());
}
FCIMPLEND

/*=========================GetRegionInfoNameOffsetTable======================
**Action: Return a pointer to an item in the Region Name Offset Table in
**        RegionInfoTable.
**Returns:
**Arguments:
**Exceptions:
==============================================================================*/

FCIMPL0(NameOffsetItem*, COMNlsInfo::nativeGetRegionInfoNameOffsetTable) {
    _ASSERTE(RegionInfoTable::GetInstance());
    return (RegionInfoTable::GetInstance()->GetNameOffsetTable());
}
FCIMPLEND

/*=======================nativeGetRegionDataFromID=============================
**Action: Given a Region ID, return the index which points to
**        the corresponding record in Region Data Table.  The index is referred
**        as 'Region Data Item Index' in the code.
**Returns: an int index points to a record in Region Data Table.  If no corresponding
**         index to return (because the Region ID is valid), -1 is returned.
**Arguments:
**         Region ID the specified Region ID.
**Exceptions: None.
==============================================================================*/

FCIMPL1(INT32,COMNlsInfo::nativeGetRegionDataFromID, INT32 nRegionID) {
    _ASSERTE(RegionInfoTable::GetInstance());
    return (RegionInfoTable::GetInstance()->GetDataItem(nRegionID));
}
FCIMPLEND

/*=============================nativeGetRegionInt32Value========================
**Action: Return the WORD value for a specific information of a Region.
**        This is used to query values like 'the number of decimal digits' for
**        a Region.
**Returns: An int for the required value (However, the value is always in WORD range).
**Arguments:
**      nRegionDataItem    the Region Data Item index.  This is an index 
**                          which points to the corresponding record in the
**                          Region Data Table.
**      nValueField  an integer indicating which fields that we are interested.
**                  See RegionInfoTable.h for a list of the fields.
**Exceptions:
==============================================================================*/

FCIMPL3(INT32, COMNlsInfo::nativeGetRegionInt32Value, INT32 CultureDataItem, INT32 ValueField, BYTE UseUserOverride)
{
    _ASSERTE(RegionInfoTable::GetInstance());
    INT32 iRetVal;
    HELPER_METHOD_FRAME_BEGIN_RET_0();
    iRetVal = RegionInfoTable::GetInstance()->GetInt32Data(CultureDataItem, ValueField, UseUserOverride);
    HELPER_METHOD_FRAME_END();
    return iRetVal;
}
FCIMPLEND

FCIMPL3(Object*, COMNlsInfo::nativeGetRegionStringValue, INT32 CultureDataItem, INT32 ValueField, BYTE UseUserOverride)
{
    _ASSERTE(RegionInfoTable::GetInstance());

    STRINGREF refRetVal = NULL;
    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, refRetVal);

    WCHAR InfoStr[MAX_STRING_VALUE];
    LPWSTR pStringValue = RegionInfoTable::GetInstance()->GetStringData(
        CultureDataItem, ValueField, UseUserOverride, InfoStr, MAX_STRING_VALUE);
    refRetVal = COMString::NewString(pStringValue);

    HELPER_METHOD_FRAME_END();
    return OBJECTREFToObject(refRetVal);
}
FCIMPLEND

/*=================================nativeInitCalendarTable============================
**Action: Create the default instance of CalendarTable.
**Returns: void.
**Arguments: void.
**Exceptions:
**      OutOfMemoryException if the creation fails.
==============================================================================*/

FCIMPL0(void, COMNlsInfo::nativeInitCalendarTable)
{
    HELPER_METHOD_FRAME_BEGIN_0();
    CalendarTable::CreateInstance();
    HELPER_METHOD_FRAME_END();
}
FCIMPLEND

/*==========================GetCalendarStringPoolTable======================
**Action: Return a pointer to the String Pool Table string in CalendarTable.
**        No range checking of any sort is performed.
**Returns:
**Arguments:
**Exceptions:
==============================================================================*/

FCIMPL1(LPWSTR, COMNlsInfo::nativeGetCalendarStringPoolStr, INT32 offset) {
    _ASSERTE(CalendarTable::GetInstance());
    _ASSERTE(offset >= 0);
    return (CalendarTable::GetInstance()->GetStringPoolTable() + offset);
}
FCIMPLEND

/*=========================nativeGetCalendarHeader======================
**Action: Return a pointer to the header in
**        CalendarTable.
**Returns:
**Arguments:
**Exceptions:
==============================================================================*/

FCIMPL0(CultureInfoHeader*, COMNlsInfo::nativeGetCalendarHeader) {
    _ASSERTE(CalendarTable::GetInstance());
    return (CalendarTable::GetInstance()->GetHeader());
}
FCIMPLEND

/*=============================nativeGetCalendarInt32Value========================
**Action: Return the WORD value for a specific information of a Calendar.
**        This is used to query values like 'the number of decimal digits' for
**        a Calendar.
**Returns: An int for the required value (However, the value is always in WORD range).
**Arguments:
**      nCalendarDataItem    the Calendar Data Item index.  This is an index 
**                          which points to the corresponding record in the
**                          Calendar Data Table.
**      nValueField  an integer indicating which fields that we are interested.
**                  See CalendarTable.h for a list of the fields.
**Exceptions:
==============================================================================*/

FCIMPL3(INT32,   COMNlsInfo::nativeGetCalendarInt32Value, INT32 CultureDataItem, INT32 ValueField, BYTE UseUserOverride)
{
    _ASSERTE(CalendarTable::GetInstance());

    INT32 iRetVal;
    HELPER_METHOD_FRAME_BEGIN_RET_0();
    iRetVal = (CalendarTable::GetInstance()->GetDefaultInt32Data(CultureDataItem, ValueField));
    HELPER_METHOD_FRAME_END();
    return iRetVal;
}
FCIMPLEND

FCIMPL3(Object*, COMNlsInfo::nativeGetCalendarStringValue, INT32 CultureDataItem, INT32 ValueField, BYTE UseUserOverride)
{
    _ASSERTE(CalendarTable::GetInstance());

    STRINGREF refRetVal = NULL;
    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, refRetVal);

    LPWSTR pInfoStr = CalendarTable::GetInstance()->GetDefaultStringData(CultureDataItem, ValueField);
    if (pInfoStr != NULL) 
    {
        refRetVal = COMString::NewString(pInfoStr);
    }
    HELPER_METHOD_FRAME_END();
    return OBJECTREFToObject(refRetVal);
}
FCIMPLEND

FCIMPL3(Object*, COMNlsInfo::nativeGetCalendarMultiStringValues, INT32 CultureDataItem, INT32 ValueField, BYTE UseUserOverride)
{   
    PTRARRAYREF refRetVal = NULL;
    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, refRetVal);

    WCHAR InfoStr[MAX_STRING_VALUE];
    LPWSTR pStringValue = CalendarTable::GetInstance()->GetStringData(
        CultureDataItem, ValueField, UseUserOverride, InfoStr, MAX_STRING_VALUE);    
    
    refRetVal = GetMultiStringValues(pStringValue);

    HELPER_METHOD_FRAME_END();
    return OBJECTREFToObject(refRetVal);
}
FCIMPLEND

//
// This method is only called by Taiwan localized build.
// 
FCIMPL2(Object*, COMNlsInfo::nativeGetEraName, INT32 nValue1, INT32 nValue2)
{
    STRINGREF refRetVal = NULL;
    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, refRetVal);

    int culture = nValue1;


    int size;
    WCHAR eraName[64];
    size = WszGetDateFormat(culture, DATE_USE_ALT_CALENDAR , NULL, L"gg", eraName, sizeof(eraName)/sizeof(WCHAR));
    if (size == 0) {
        goto lEmptyString;
    }

    refRetVal = AllocateString(size);
    wcscpy(refRetVal->GetBuffer(), eraName);
    refRetVal->SetStringLength(size - 1);
    goto lExit;

lEmptyString:
    // Return an empty string.
    refRetVal = COMString::NewString(0);

lExit: ;
    HELPER_METHOD_FRAME_END();
    return OBJECTREFToObject(refRetVal);
}
FCIMPLEND


/*=================================nativeInitRegionInfoTable============================
**Action: Create the default instance of RegionInfoTable.
**Returns: void.
**Arguments: void.
**Exceptions:
**      OutOfMemoryException if the creation fails.
==============================================================================*/

FCIMPL0(VOID, COMNlsInfo::nativeInitUnicodeCatTable) 
{
    THROWSCOMPLUSEXCEPTION();

    HELPER_METHOD_FRAME_BEGIN_0();

    CharacterInfoTable* pTable = CharacterInfoTable::CreateInstance();
    if (pTable == NULL) 
    {
        COMPlusThrowOM();
    }

    HELPER_METHOD_FRAME_END();
}
FCIMPLEND

FCIMPL0(LPVOID, COMNlsInfo::nativeGetUnicodeCatTable) {
    _ASSERTE(CharacterInfoTable::GetInstance());
    return (LPVOID)(CharacterInfoTable::GetInstance()->GetCategoryDataTable());
}
FCIMPLEND

BYTE COMNlsInfo::GetUnicodeCategory(WCHAR wch) {
    THROWSCOMPLUSEXCEPTION();
    CharacterInfoTable* pTable = CharacterInfoTable::CreateInstance();
    if (pTable == NULL) {
        COMPlusThrowOM();
    }
    return (pTable->GetUnicodeCategory(wch));
}

BOOL COMNlsInfo::nativeIsWhiteSpace(WCHAR c) {
    // This is the native equivalence of CharacterInfo.IsWhiteSpace().
    switch (c) {
        case ' ':
        case '\x0009' :
        case '\x000a' :
        case '\x000b' :
        case '\x000c' :
        case '\x000d' :
        case '\x0085' :
            return (TRUE);
    }
      
    BYTE uc = GetUnicodeCategory(c);
    switch (uc) {
        case (11):      // UnicodeCategory.SpaceSeparator
        case (12):      // UnicodeCategory.LineSeparator
        case (13):      // UnicodeCategory.ParagraphSeparator
            return (TRUE);    
    }
    return (FALSE);
}


FCIMPL0(LPVOID, COMNlsInfo::nativeGetUnicodeCatLevel2Offset) {
    _ASSERTE(CharacterInfoTable::GetInstance());
    return (LPVOID)(CharacterInfoTable::GetInstance()->GetCategoryLevel2OffsetTable());
}
FCIMPLEND

FCIMPL0(LPVOID, COMNlsInfo::nativeGetNumericTable) {
    _ASSERTE(CharacterInfoTable::GetInstance());
    return (LPVOID)(CharacterInfoTable::GetInstance()->GetNumericDataTable());
}
FCIMPLEND

FCIMPL0(LPVOID, COMNlsInfo::nativeGetNumericLevel2Offset) {
    _ASSERTE(CharacterInfoTable::GetInstance());
    return (LPVOID)(CharacterInfoTable::GetInstance()->GetNumericLevel2OffsetTable());
}
FCIMPLEND

FCIMPL0(LPVOID, COMNlsInfo::nativeGetNumericFloatData) {
    _ASSERTE(CharacterInfoTable::GetInstance());
    return (LPVOID)(CharacterInfoTable::GetInstance()->GetNumericFloatData());
}
FCIMPLEND

FCIMPL0(INT32, COMNlsInfo::nativeGetThreadLocale)
{
    return (::GetThreadLocale());
}
FCIMPLEND

FCIMPL1(BOOL, COMNlsInfo::nativeSetThreadLocale, INT32 lcid)
{
    return ::SetThreadLocale(lcid);
}
FCIMPLEND


/*============================ConvertStringCaseFast=============================
**Action:
**Returns:
**Arguments:
**Exceptions:
==============================================================================*/
void COMNlsInfo::ConvertStringCaseFast(WCHAR *inBuff, WCHAR *outBuff, INT32 length, DWORD dwOptions) {
        if (dwOptions&LCMAP_UPPERCASE) {
            for (int i=0; i<length; i++) {
                _ASSERTE(inBuff[i]<0x80);
                outBuff[i]=ToUpperMapping[inBuff[i]];
            }
        } else if (dwOptions&LCMAP_LOWERCASE) {
            for (int i=0; i<length; i++) {
                _ASSERTE(inBuff[i]<0x80);
                outBuff[i]=ToLowerMapping[inBuff[i]];
            }    
        }
}


/*==============================DoComparisonLookup==============================
**Action:
**Returns:
**Arguments:
**Exceptions:
==============================================================================*/
INT32 COMNlsInfo::DoComparisonLookup(wchar_t charA, wchar_t charB) {
    
    if ((charA ^ charB) & 0x20) {
        //We may be talking about a special case
        if (charA>='A' && charA<='Z') {
            return 1;
        }

        if (charA>='a' && charA<='z') {
            return -1;
        }
    }

    if (charA==charB) {
        return 0;
    }

    return ((charA>charB)?1:-1);
}


/*================================DoCompareChars================================
**Action:
**Returns:
**Arguments:
**Exceptions:
==============================================================================*/
FORCEINLINE INT32 COMNlsInfo::DoCompareChars(WCHAR charA, WCHAR charB, BOOL *bDifferInCaseOnly) {
    INT32 result;
    WCHAR temp;

    //The ComparisonTable is a 0x80 by 0x80 table of all of the characters in which we're interested
    //and their sorting value relative to each other.  We can do a straight lookup to get this info.
    result = ComparisonTable[(int)(charA)][(int)(charB)];
    
    //This is the tricky part of doing locale-aware sorting.  Case-only differences only matter in the
    //event that they're the only difference in the string.  We mark characters that differ only in case
    //and deal with the rest of the logic in CompareFast.
    *bDifferInCaseOnly = (((charA ^ 0x20)==charB) && (((temp=(charA | 0x20))>='a') && (temp<='z')));
    return result;
}


/*=================================CompareFast==================================
**Action:
**Returns:
**Arguments:
**Exceptions:
==============================================================================*/
INT32 COMNlsInfo::CompareFast(STRINGREF strA, STRINGREF strB, BOOL *pbDifferInCaseOnly) {
    WCHAR *charA, *charB;
    DWORD *dwAChars, *dwBChars;
    INT32 strALength, strBLength;
    BOOL bDifferInCaseOnly=false;
    BOOL bDifferTemp;
    INT32 caseOnlyDifference=0;
    INT32 result;

    RefInterpretGetStringValuesDangerousForGC(strA, (WCHAR **) &dwAChars, &strALength);
    RefInterpretGetStringValuesDangerousForGC(strB, (WCHAR **) &dwBChars, &strBLength);

    *pbDifferInCaseOnly = false;

    // If the strings are the same length, compare exactly the right # of chars.
    // If they are different, compare the shortest # + 1 (the '\0').
    int count = strALength;
    if (count > strBLength)
        count = strBLength;
    
    ptrdiff_t diff = (char *)dwAChars - (char *)dwBChars;

    int c;
    //Compare the characters a DWORD at a time.  If they differ at all, examine them
    //to find out which character (or both) is different.  The actual work of doing the comparison
    //is done in DoCompareChars.  If they differ in case only, we need to track this, but keep looking
    //in case there's someplace where they actually differ in more than case.  This is counterintuitive to
    //most devs, but makes sense if you consider the case where the strings are being sorted to be displayed.
    while ((count-=2)>=0) {
        if ((c = *((DWORD* )((char *)dwBChars + diff)) - *dwBChars) != 0) {
            charB = (WCHAR *)dwBChars;
            charA = ((WCHAR* )((char *)dwBChars + diff));
            if (*charA!=*charB) {
                result = DoCompareChars(*charA, *charB, &bDifferTemp);
                //We know that the two characters are different because of the check that we did before calling DoCompareChars.
                //If they don't differ in just case, we've found the difference, so we can return that.
                if (!bDifferTemp) {
                    return result;
                }

                //We only note the difference the first time that they differ in case only.  If we haven't seen a case-only
                //difference before, we'll record the difference and set bDifferInCaseOnly to true and record the difference.
                if (!bDifferInCaseOnly) {
                    bDifferInCaseOnly = true;
                    caseOnlyDifference=result;
                }
            }
            // Two cases will get us here: The first chars are the same or
            // they differ in case only.
            // The logic here is identical to the logic described above.
            charA++; charB++;
            if (*charA!=*charB) {
                result = DoCompareChars(*charA, *charB, &bDifferTemp);
                if (!bDifferTemp) {
                    return result;
                }
                if (!bDifferInCaseOnly) {
                    bDifferInCaseOnly = true;
                    caseOnlyDifference=result;
                }
            }
        }
        ++dwBChars;
    }

    //We'll only get here if we had an odd number of characters.  If we did, repeat the logic from above for the last
    //character in the string.
    if (count == -1) {
        charB = (WCHAR *)dwBChars;
        charA = ((WCHAR* )((char *)dwBChars + diff));
        if (*charA!=*charB) {
            result = DoCompareChars(*charA, *charB, &bDifferTemp);
            if (!bDifferTemp) {
                return result;
            }
            if (!bDifferInCaseOnly) {
                bDifferInCaseOnly = true;
                caseOnlyDifference=result;
            }
        }
    }

    //If the lengths are the same, return the case-only difference (if such a thing exists).  
    //Otherwise just return the longer string.
    if (strALength==strBLength) {
        if (bDifferInCaseOnly) {
            *pbDifferInCaseOnly = true;
            return caseOnlyDifference;
        } 
        return 0;
    }
    
    return (strALength>strBLength)?1:-1;
}


INT32 COMNlsInfo::CompareOrdinal(WCHAR* string1, int Length1, WCHAR* string2, int Length2 )
{
    //
    // NOTENOTE The code here should be in sync with COMString::FCCompareOrdinal
    //
    DWORD *strAChars, *strBChars;
    strAChars = (DWORD*)string1;
    strBChars = (DWORD*)string2;

    // If the strings are the same length, compare exactly the right # of chars.
    // If they are different, compare the shortest # + 1 (the '\0').
    int count = Length1;
    if (count > Length2)
        count = Length2;
    ptrdiff_t diff = (char *)strAChars - (char *)strBChars;

    // Loop comparing a DWORD at a time.
    while ((count -= 2) >= 0)
    {
        if ((*((DWORD* )((char *)strBChars + diff)) - *strBChars) != 0)
        {
            LPWSTR ptr1 = (WCHAR*)((char *)strBChars + diff);
            LPWSTR ptr2 = (WCHAR*)strBChars;
            if (*ptr1 != *ptr2) {
                return ((int)*ptr1 - (int)*ptr2);
            }
            return ((int)*(ptr1+1) - (int)*(ptr2+1));
        }
        ++strBChars;
    }

    int c;
    // Handle an extra WORD.
    if (count == -1)
        if ((c = *((WCHAR *) ((char *)strBChars + diff)) - *((WCHAR *) strBChars)) != 0)
            return c;            
    return Length1 - Length2;
}

////////////////////////////////////////////////////////////////////////////
//
//  Compare
//
////////////////////////////////////////////////////////////////////////////

FCIMPL5(INT32, COMNlsInfo::Compare,
    INT_PTR         pNativeCompareInfo,
    INT32           LCID,
    StringObject*   pString1UNSAFE,
    StringObject*   pString2UNSAFE,
    INT32           dwFlags
    )
{
    THROWSCOMPLUSEXCEPTION();

    INT32       dwRetVal = 0;
    STRINGREF   pString1 = ObjectToSTRINGREF(pString1UNSAFE);
    STRINGREF   pString2 = ObjectToSTRINGREF(pString2UNSAFE);

    //Our paradigm is that null sorts less than any other string and 
    //that two nulls sort as equal.
    if (pString1 == NULL) {
        if (pString2 == NULL) {
            return (0);     // Equal
        }
        return (-1);    // null < non-null
    }
    if (pString2 == NULL) {
        return (1);     // non-null > null
    }

    HELPER_METHOD_FRAME_BEGIN_RET_2(pString1, pString2);

    //
    //  Check the parameters.
    //
    
    if (dwFlags < 0) 
    {
        COMPlusThrowArgumentOutOfRange(L"flags", L"ArgumentOutOfRange_MustBePositive");
    }

    //
    // Check if we can use the highly optimized comparisons
    //

    bool bFastCompareLocale = IS_FAST_COMPARE_LOCALE(LCID);

    if (bFastCompareLocale) 
    {
        //If we've never before looked at whether this string has high chars, do so now.
        if (IS_STRING_STATE_UNDETERMINED(pString1->GetHighCharState())) 
        {
            COMString::InternalCheckHighChars(pString1);
        }
        
        //If we've never before looked at whether this string has high chars, do so now.
        if (IS_STRING_STATE_UNDETERMINED(pString2->GetHighCharState())) 
        {
            COMString::InternalCheckHighChars(pString2);
        }
    }
        
    if ((bFastCompareLocale) &&
        (IS_FAST_SORT(pString1->GetHighCharState())) &&
        (IS_FAST_SORT(pString2->GetHighCharState())) &&
        (dwFlags<=1)) 
    {
            //0 is no flags.  1 is ignore case.  We can handle both here.
            BOOL bDifferInCaseOnly;
        dwRetVal = CompareFast(pString1, pString2, &bDifferInCaseOnly);
            
        if (dwFlags != 0) //If we're looking to do a case-sensitive comparison
        { 
            //The remainder of this block deals with instances where we're ignoring case.
            if (bDifferInCaseOnly) 
            {
                dwRetVal = 0;
            } 
        }
    }
    else
    {
        if (dwFlags & COMPARE_OPTIONS_ORDINAL) 
        {
            if (dwFlags == COMPARE_OPTIONS_ORDINAL) 
            {
                //
                // Ordinal means the code-point comparison.  This option can not be
                // used with other options.
                // 
                
                //
                //  Compare the two strings to the length of the shorter string.
                //  If they're not equal lengths, and the heads are equal, then the
                //  longer string is greater.
                //
                dwRetVal = CompareOrdinal(pString1->GetBuffer(), 
                                          pString1->GetStringLength(), 
                                          pString2->GetBuffer(), 
                                          pString2->GetStringLength());
            } 
            else 
            {
                COMPlusThrowArgumentException(L"options", L"Argument_CompareOptionOrdinal");
            }
        }
        else
        {

        // The return value of NativeCompareInfo::CompareString() is Win32-style value (1=less, 2=equal, 3=larger).
        // So substract by two to get the NLS+ value.
        // Will change NativeCompareInfo to return the correct value later s.t. we don't have
        // to subtract 2.

        // NativeCompareInfo::CompareString() won't take -1 as the end of string anymore.  Therefore,
        // pass the correct string length.
        // The change is for adding the null-embeded string support in CompareString().
        //
            dwRetVal = (((NativeCompareInfo*)(pNativeCompareInfo))->CompareString(
                dwFlags, 
                pString1->GetBuffer(), 
                pString1->GetStringLength(), 
                pString2->GetBuffer(), 
                pString2->GetStringLength()) - 2);
        }
    }

    HELPER_METHOD_FRAME_END();

    return dwRetVal;
}
FCIMPLEND

////////////////////////////////////////////////////////////////////////////
//
//  CompareRegion
//
////////////////////////////////////////////////////////////////////////////

FCIMPL9(INT32, COMNlsInfo::CompareRegion, 
                    INT_PTR         pNativeCompareInfo,
                    INT32           LCID,
                    StringObject*   pString1,
                    INT32           Offset1,
                    INT32           Length1,
                    StringObject*   pString2,
                    INT32           Offset2,
                    INT32           Length2,
                    INT32           dwFlags)
{
    THROWSCOMPLUSEXCEPTION();

    //
    // Check for the null case.
    //
    if (pString1 == NULL) {
        if (Offset1 != 0 || (Length1 != 0 && Length1 != -1)) {
            FCThrowArgumentOutOfRange(L"string1", L"ArgumentOutOfRange_OffsetLength");
        }
        if (pString2 == NULL) {
            if (Offset2 != 0 || (Length2 != 0 && Length2 != -1)) {
                FCThrowArgumentOutOfRange( L"string2", L"ArgumentOutOfRange_OffsetLength");
            }
            return (0);
        }
        return (-1);
    }
    if (pString2 == NULL) {
        if (Offset2 != 0 || (Length2 != 0 && Length2 != -1)) {
            FCThrowArgumentOutOfRange(L"string2", L"ArgumentOutOfRange_OffsetLength");
        }
        return (1);
    }
    //
    //  Get the full length of the two strings.
    //
    int realLen1 = pString1->GetStringLength();
    int realLen2 = pString2->GetStringLength();

    //check the arguments.
    // Criteria:
    // OffsetX >= 0
    // LengthX >= 0 || LengthX == -1 (that is, LengthX >= -1)
    // If LengthX >= 0, OffsetX + LengthX <= realLenX
    if (Offset1<0) {
        FCThrowArgumentOutOfRange(L"offset1", L"ArgumentOutOfRange_Index");
    }
    if (Offset2<0) {
        FCThrowArgumentOutOfRange(L"offset2", L"ArgumentOutOfRange_Index");
    }
    if (Length1 >= 0 && Length1>realLen1 - Offset1) {
        FCThrowArgumentOutOfRange(L"string1", L"ArgumentOutOfRange_OffsetLength");
    }
    if (Length2 >= 0 && Length2>realLen2 - Offset2){ 
        FCThrowArgumentOutOfRange(L"string2", L"ArgumentOutOfRange_OffsetLength");
    }

    // NativeCompareInfo::CompareString() won't take -1 as the end of string anymore.  Therefore,
    // pass the correct string length.
    // The change is for adding the null-embeded string support in CompareString().
    // Therefore, if the length is -1, we have to get the correct string length here.
    //
    if (Length1 == -1) {
        Length1 = realLen1 - Offset1;
    }

    if (Length2 == -1) {
        Length2 = realLen2 - Offset2;
    }

    if (Length1 < 0) {
       FCThrowArgumentOutOfRange(L"length1", L"ArgumentOutOfRange_NegativeLength");
    }
    if (Length2 < 0) {
       FCThrowArgumentOutOfRange(L"length2", L"ArgumentOutOfRange_NegativeLength");
    }
    
    if (dwFlags == COMPARE_OPTIONS_ORDINAL)        
    {
        return (CompareOrdinal(
                    pString1->GetBuffer()+Offset1, 
                    Length1, 
                    pString2->GetBuffer()+Offset2, 
                    Length2));    
    }

    return (((NativeCompareInfo*)(pNativeCompareInfo))->CompareString(
        dwFlags, 
        pString1->GetBuffer() + Offset1, 
        Length1, 
        pString2->GetBuffer() + Offset2, 
        Length2) - 2);
}
FCIMPLEND


////////////////////////////////////////////////////////////////////////////
//
//  IndexOfChar
//
////////////////////////////////////////////////////////////////////////////

FCIMPL7(INT32, COMNlsInfo::IndexOfChar, INT_PTR pNativeCompareInfo, INT32 myLCID, StringObject* pStringUNSAFE,  WCHAR ch, INT32 StartIndex, INT32 Count, INT32 dwFlags)
{
    THROWSCOMPLUSEXCEPTION();

    INT32     iRetVal = -1;
    STRINGREF pString = (STRINGREF) pStringUNSAFE;

    HELPER_METHOD_FRAME_BEGIN_RET_1(pString)

    int EndIndex = 0;
    WCHAR *buffer = NULL;
    BOOL bASCII = FALSE;

    //
    //  Make sure there is a string.
    //
    if (!pString) {
        COMPlusThrowArgumentNull(L"string",L"ArgumentNull_String");
    }
    //
    //  Get the arguments.
    //
    int StringLength = pString->GetStringLength();

    //
    //  Check the ranges.
    //
    if (StringLength == 0)
    {
        goto lExit;
    }
    
    if (StartIndex<0 || StartIndex> StringLength) {
        COMPlusThrowArgumentOutOfRange(L"startIndex", L"ArgumentOutOfRange_Index");
    }

    if (Count == -1)
    {
        Count = StringLength - StartIndex;        
    }
    else
    {
        if ((Count < 0) || (Count > StringLength - StartIndex))
        {
            COMPlusThrowArgumentOutOfRange(L"count", L"ArgumentOutOfRange_Count");        
        }
      }

    //
    //  Search for the character in the string starting at StartIndex.

    EndIndex = StartIndex + Count - 1;
    buffer = pString->GetBuffer();
    int ctr;

    if (dwFlags!=COMPARE_OPTIONS_ORDINAL) {
        //
        // Check if we can use the highly optimized comparisons
        //
        
        //If we've never before looked at whether this string has high chars, do so now.
        if (IS_STRING_STATE_UNDETERMINED(pString->GetHighCharState())) {
            COMString::InternalCheckHighChars(pString);
        }
        
        bASCII = ((IS_FAST_INDEX(pString->GetHighCharState())) && ch < 0x7f) || (ch == 0);
    }

    if ((bASCII && dwFlags == 0) || (dwFlags == COMPARE_OPTIONS_ORDINAL))
    {
        for (ctr = StartIndex; ctr <= EndIndex; ctr++)
        {
            if (buffer[ctr] == ch)
            {
                iRetVal = ctr;
                goto lExit;
            }
        }
        goto lExit;
    } 
    else if (bASCII && dwFlags == COMPARE_OPTIONS_IGNORECASE)
    {
        WCHAR chctr= 0;
        WCHAR UpperValue = (ch>='A' && ch<='Z')?(ch|0x20):ch;

        for (ctr = StartIndex; ctr <= EndIndex; ctr++)
        {
            chctr = buffer[ctr];
            chctr = (chctr>='A' && chctr<='Z')?(chctr|0x20):chctr;

            if (UpperValue == chctr) {
                iRetVal = ctr;
                goto lExit;
            }
        }
        goto lExit;
    } 
    iRetVal = ((NativeCompareInfo*)(pNativeCompareInfo))->IndexOfString(
        pString->GetBuffer(), &ch, StartIndex, EndIndex, 1, dwFlags, FALSE);
    if (iRetVal == INDEXOF_INVALID_FLAGS) {
        COMPlusThrowArgumentException(L"flags", L"Argument_InvalidFlag");
    }

lExit: ;
    HELPER_METHOD_FRAME_END();
    return iRetVal;
}
FCIMPLEND


////////////////////////////////////////////////////////////////////////////
//
//  LastIndexOfChar
//
////////////////////////////////////////////////////////////////////////////

FCIMPL7(INT32, COMNlsInfo::LastIndexOfChar, INT_PTR pNativeCompareInfo, INT32 myLCID, StringObject* pStringUNSAFE,  WCHAR ch, INT32 StartIndex, INT32 Count, INT32 dwFlags)
{
    THROWSCOMPLUSEXCEPTION();

    INT32     iRetVal = -1;
    STRINGREF pString = (STRINGREF) pStringUNSAFE;
    HELPER_METHOD_FRAME_BEGIN_RET_1(pString);

    LCID Locale = 0;
    WCHAR *buffer = NULL;
    BOOL bASCII = FALSE;

    //
    //  Make sure there is a string.
    //
    if (!pString) {
        COMPlusThrowArgumentNull(L"string",L"ArgumentNull_String");
    }
    //
    //  Get the arguments.
    //
    int StringLength = pString->GetStringLength();

    //
    //  Check the ranges.
    //
    if (StringLength == 0)
    {
        goto lExit;
    }

    if ((StartIndex < 0) || (StartIndex > StringLength))
    {
        COMPlusThrowArgumentOutOfRange(L"startIndex", L"ArgumentOutOfRange_Index");
    }

    int EndIndex;
    if (Count == -1)
    {
        EndIndex = 0;
    }
    else
    {
        if ((Count < 0) || (Count > StartIndex + 1))
        {
            COMPlusThrowArgumentOutOfRange(L"count", L"ArgumentOutOfRange_Count");
        }
        EndIndex = StartIndex - Count + 1;
    }    

    //
    //  Search for the character in the string starting at EndIndex.
    Locale = myLCID;
    buffer = pString->GetBuffer();
    int ctr;

    //If they haven't asked for exact comparison, we may still be able to do a 
    //fast comparison if the string is all less than 0x80. 
    if (dwFlags!=COMPARE_OPTIONS_ORDINAL) {
        //If we've never before looked at whether this string has high chars, do so now.
        if (IS_STRING_STATE_UNDETERMINED(pString->GetHighCharState())) {
            COMString::InternalCheckHighChars(pString);
        }

        bASCII = (IS_FAST_INDEX(pString->GetHighCharState()) && ch < 0x7f) || (ch == 0);
    }

    if ((bASCII && dwFlags == 0) || (dwFlags == COMPARE_OPTIONS_ORDINAL))
    {
        for (ctr = StartIndex; ctr >= EndIndex; ctr--)
        {
            if (buffer[ctr] == ch)
            {
                iRetVal = ctr;
                goto lExit;
            }
        }
        goto lExit;
    }
    else if (bASCII && dwFlags == COMPARE_OPTIONS_IGNORECASE)
    {
        WCHAR UpperValue = (ch>='A' && ch<='Z')?(ch|0x20):ch;
        WCHAR chctr;

        for (ctr = StartIndex; ctr >= EndIndex; ctr--)
        {
            chctr = buffer[ctr];
            chctr = (chctr>='A' && chctr<='Z')?(chctr|0x20):chctr;
            
            if (UpperValue == chctr) {
                iRetVal = ctr;
                goto lExit;
            }
        }
        goto lExit;
    }
    int nMatchEndIndex;
    iRetVal = ((NativeCompareInfo*)(pNativeCompareInfo))->LastIndexOfString(
        pString->GetBuffer(), &ch, StartIndex, EndIndex, 1, dwFlags, &nMatchEndIndex);
    if (iRetVal == INDEXOF_INVALID_FLAGS) {
        COMPlusThrowArgumentException(L"flags", L"Argument_InvalidFlag");
    }
    
lExit: ;
    HELPER_METHOD_FRAME_END();
    return iRetVal;
}
FCIMPLEND

INT32 COMNlsInfo::FastIndexOfString(WCHAR *source, INT32 startIndex, INT32 endIndex, WCHAR *pattern, INT32 patternLength) 
{
    int endPattern = endIndex - patternLength + 1;
    
    if (endPattern<0) {
        return -1;
    }

    if (patternLength <= 0) {
        return startIndex;
    }

    WCHAR patternChar0 = pattern[0];
    for (int ctrSrc = startIndex; ctrSrc<=endPattern; ctrSrc++) {
        if (source[ctrSrc] != patternChar0)
            continue;
        int ctrPat;
        for (ctrPat = 1; (ctrPat < patternLength) && (source[ctrSrc + ctrPat] == pattern[ctrPat]); ctrPat++) {
            ;
        }
        if (ctrPat == patternLength) {
            return (ctrSrc);
        }
    }
    
    return (-1);
}

INT32 COMNlsInfo::FastIndexOfStringInsensitive(WCHAR *source, INT32 startIndex, INT32 endIndex, WCHAR *pattern, INT32 patternLength) {
    WCHAR srcChar;
    WCHAR patChar;

    int endPattern = endIndex - patternLength + 1;
    
    if (endPattern<0) {
        return -1;
    }

    for (int ctrSrc = startIndex; ctrSrc<=endPattern; ctrSrc++) {
        int ctrPat;
        for (ctrPat = 0; (ctrPat < patternLength); ctrPat++) {
            srcChar = source[ctrSrc + ctrPat];
            if (srcChar>='A' && srcChar<='Z') {
                srcChar|=0x20;
            }
            patChar = pattern[ctrPat];
            if (patChar>='A' && patChar<='Z') {
                patChar|=0x20;
            }
            if (srcChar!=patChar) {
                break;
            }
        }

        if (ctrPat == patternLength) {
            return (ctrSrc);
        }
    }
    
    return (-1);
}


////////////////////////////////////////////////////////////////////////////
//
//  IndexOfString
//
////////////////////////////////////////////////////////////////////////////

FCIMPL7(INT32, COMNlsInfo::IndexOfString,
                    INT_PTR pNativeCompareInfo,
                    INT32 LCID,
                    StringObject* pString1UNSAFE,
                    StringObject* pString2UNSAFE,
                    INT32 StartIndex,
                    INT32 Count,
                    INT32 dwFlags)
{
    THROWSCOMPLUSEXCEPTION();

    INT32       dwRetVal = 0;
    STRINGREF   pString1 = ObjectToSTRINGREF(pString1UNSAFE);
    STRINGREF   pString2 = ObjectToSTRINGREF(pString2UNSAFE);

    HELPER_METHOD_FRAME_BEGIN_RET_2(pString1, pString2);

    int EndIndex = 0;
    WCHAR *Buffer1 = NULL;
    WCHAR *Buffer2 = NULL;

    //
    //  Make sure there are strings.
    //
    if ((pString1 == NULL) || (pString2 == NULL))
    {
        COMPlusThrowArgumentNull((pString1 == NULL ? L"string1": L"string2"),L"ArgumentNull_String");
    }
    //
    //  Get the arguments.
    //
    int StringLength1 = pString1->GetStringLength();
    int StringLength2 = pString2->GetStringLength();

    //
    //  Check the ranges.
    //
    if (StringLength1 == 0)
    {
        if (StringLength2 == 0) 
        {
            dwRetVal = 0;
        }
        else
        {
            dwRetVal = -1;
        }
        goto lExit;
    }

    if ((StartIndex < 0) || (StartIndex > StringLength1))
    {
        COMPlusThrowArgumentOutOfRange(L"startIndex", L"ArgumentOutOfRange_Index");
    }

    if (Count == -1)
    {
        Count = StringLength1 - StartIndex;
    }
    else
    {
        if ((Count < 0) || (Count > StringLength1 - StartIndex))
        {
            COMPlusThrowArgumentOutOfRange(L"count", L"ArgumentOutOfRange_Count");
        }
    }

    //
    //  See if we have an empty string 2.
    //
    if (StringLength2 == 0)
    {
        dwRetVal = StartIndex;
        goto lExit;
    }

    EndIndex = StartIndex + Count - 1;

    //
    //  Search for the character in the string.
    //
    Buffer1 = pString1->GetBuffer();
    Buffer2 = pString2->GetBuffer();

    if (dwFlags == COMPARE_OPTIONS_ORDINAL) 
    {
        dwRetVal = FastIndexOfString(Buffer1, StartIndex, EndIndex, Buffer2, StringLength2);
        goto lExit;
    }

    //For dwFlags, 0 is the default, 1 is ignore case, we can handle both.
    if (dwFlags<=1 && IS_FAST_COMPARE_LOCALE(LCID)) 
    {
        //If we've never before looked at whether this string has high chars, do so now.
        if (IS_STRING_STATE_UNDETERMINED(pString1->GetHighCharState())) 
        {
            COMString::InternalCheckHighChars(pString1);
        }
        
        //If we've never before looked at whether this string has high chars, do so now.
        if (IS_STRING_STATE_UNDETERMINED(pString2->GetHighCharState())) 
        {
            COMString::InternalCheckHighChars(pString2);
        }

        //If neither string has high chars, we can use a much faster comparison algorithm.
        if (IS_FAST_INDEX(pString1->GetHighCharState()) && IS_FAST_INDEX(pString2->GetHighCharState())) 
        {
            if (dwFlags==0) 
            {
                dwRetVal = FastIndexOfString(Buffer1, StartIndex, EndIndex, Buffer2, StringLength2);
                goto lExit;
            } 
            else 
            {
                dwRetVal = FastIndexOfStringInsensitive(Buffer1, StartIndex, EndIndex, Buffer2, StringLength2);
                goto lExit;
            }
        }
    }

    dwRetVal = ((NativeCompareInfo*)(pNativeCompareInfo))->IndexOfString(
        Buffer1, Buffer2, StartIndex, EndIndex, StringLength2, dwFlags, FALSE);
    if (dwRetVal == INDEXOF_INVALID_FLAGS) 
    {
        COMPlusThrowArgumentException(L"flags", L"Argument_InvalidFlag");
    }

lExit: ;
    HELPER_METHOD_FRAME_END();

    return dwRetVal;    
}
FCIMPLEND

INT32 COMNlsInfo::FastLastIndexOfString(WCHAR *source, INT32 startIndex, INT32 endIndex, WCHAR *pattern, INT32 patternLength) {
    //startIndex is the greatest index into the string.
    int startPattern = startIndex - patternLength + 1;
    
    if (startPattern < 0) {
        return (-1);
    }

    for (int ctrSrc = startPattern; ctrSrc >= endIndex; ctrSrc--) {
        int ctrPat;
        for (ctrPat = 0; (ctrPat<patternLength) && (source[ctrSrc+ctrPat] == pattern[ctrPat]); ctrPat++) {
            //Deliberately empty.
        }
        if (ctrPat == patternLength) {
            return (ctrSrc);
        }
    }

    return (-1);
}

INT32 COMNlsInfo::FastLastIndexOfStringInsensitive(WCHAR *source, INT32 startIndex, INT32 endIndex, WCHAR *pattern, INT32 patternLength) {
    //startIndex is the greatest index into the string.
    int startPattern = startIndex - patternLength + 1;
    WCHAR srcChar;
    WCHAR patChar;

    if (startPattern < 0) {
        return (-1);
    }

    for (int ctrSrc = startPattern; ctrSrc >= endIndex; ctrSrc--) {
        int ctrPat;
        for (ctrPat = 0; (ctrPat<patternLength); ctrPat++) {
            srcChar = source[ctrSrc+ctrPat];
            if (srcChar>='A' && srcChar<='Z') {
                srcChar|=0x20;
            }
            patChar = pattern[ctrPat];
            if (patChar>='A' && patChar<='Z') {
                patChar|=0x20;
            }
            if (srcChar!=patChar) {
                break;
            }
        }
        if (ctrPat == patternLength) {
            return (ctrSrc);
        }
    }

    return (-1);
}

// The parameter verfication is done in the managed side.
FCIMPL5(INT32, COMNlsInfo::nativeIsPrefix, INT_PTR pNativeCompareInfo, INT32 LCID, StringObject* pString1UNSAFE, StringObject* pString2UNSAFE, INT32 dwFlags) 
{
    STRINGREF pString1 = (STRINGREF) pString1UNSAFE;
    STRINGREF pString2 = (STRINGREF) pString2UNSAFE;

    int SourceLength = pString1->GetStringLength();
    int PrefixLength = pString2->GetStringLength();

    WCHAR *SourceBuffer = pString1->GetBuffer();
    WCHAR *PrefixBuffer = pString2->GetBuffer();

    if (dwFlags == COMPARE_OPTIONS_ORDINAL) {
        if (PrefixLength > SourceLength) {
            return (FALSE);
        }
        return (memcmp(SourceBuffer, PrefixBuffer, PrefixLength * sizeof(WCHAR)) == 0);
    }

    //For dwFlags, 0 is the default, 1 is ignore case, we can handle both.
    if (dwFlags<=1 && IS_FAST_COMPARE_LOCALE(LCID)) {
        //If we've never before looked at whether this string has high chars, do so now.
        if (IS_STRING_STATE_UNDETERMINED(pString1->GetHighCharState())) {
            COMString::InternalCheckHighChars(pString1);
        }
        
        //If we've never before looked at whether this string has high chars, do so now.
        if (IS_STRING_STATE_UNDETERMINED(pString2->GetHighCharState())) {
            COMString::InternalCheckHighChars(pString2);
        }

        //If neither string has high chars, we can use a much faster comparison algorithm.
        if (IS_FAST_INDEX(pString1->GetHighCharState()) && IS_FAST_INDEX(pString2->GetHighCharState())) {
            if (SourceLength < PrefixLength) {
                return (FALSE);
            }
            if (dwFlags==0) {
                return (memcmp(SourceBuffer, PrefixBuffer, PrefixLength * sizeof(WCHAR)) == 0);
            } else {
                LPCWSTR SourceEnd = SourceBuffer + PrefixLength;
                while (SourceBuffer < SourceEnd) {
                    WCHAR srcChar = *SourceBuffer;
                    if (srcChar>='A' && srcChar<='Z') {
                        srcChar|=0x20;
                    }
                    WCHAR prefixChar = *PrefixBuffer;
                    if (prefixChar>='A' && prefixChar<='Z') {
                        prefixChar|=0x20;
                    }
                    if (srcChar!=prefixChar) {
                        return (FALSE);
                    }
                    SourceBuffer++; PrefixBuffer++;
                }
                return (TRUE);
            }
        }
    }
    

    return ((NativeCompareInfo*)pNativeCompareInfo)->IsPrefix(SourceBuffer, SourceLength, PrefixBuffer, PrefixLength, dwFlags);
}
FCIMPLEND

// The parameter verfication is done in the managed side.
FCIMPL5(INT32, COMNlsInfo::nativeIsSuffix, INT_PTR pNativeCompareInfo, INT32 LCID, StringObject* pString1UNSAFE, StringObject* pString2UNSAFE, INT32 dwFlags) 
{
    STRINGREF pString1 = (STRINGREF) pString1UNSAFE;
    STRINGREF pString2 = (STRINGREF) pString2UNSAFE;

    int SourceLength = pString1->GetStringLength();
    int SuffixLength = pString2->GetStringLength();

    WCHAR *SourceBuffer = pString1->GetBuffer();
    WCHAR *SuffixBuffer = pString2->GetBuffer();

    if (dwFlags == COMPARE_OPTIONS_ORDINAL) {
        if (SuffixLength > SourceLength) {
            return (FALSE);
        }
        return (memcmp(SourceBuffer + SourceLength - SuffixLength, SuffixBuffer, SuffixLength * sizeof(WCHAR)) == 0);
    }

    //For dwFlags, 0 is the default, 1 is ignore case, we can handle both.
    if (dwFlags<=1 && IS_FAST_COMPARE_LOCALE(LCID)) {
        //If we've never before looked at whether this string has high chars, do so now.
        if (IS_STRING_STATE_UNDETERMINED(pString1->GetHighCharState())) {
            COMString::InternalCheckHighChars(pString1);
        }
        
        //If we've never before looked at whether this string has high chars, do so now.
        if (IS_STRING_STATE_UNDETERMINED(pString2->GetHighCharState())) {
            COMString::InternalCheckHighChars(pString2);
        }

        //If neither string has high chars, we can use a much faster comparison algorithm.
        if (IS_FAST_INDEX(pString1->GetHighCharState()) && IS_FAST_INDEX(pString2->GetHighCharState())) {
            int nSourceStart = SourceLength - SuffixLength;
            if (nSourceStart < 0) {
                return (FALSE);
            }
            if (dwFlags==0) {
                return (memcmp(SourceBuffer + nSourceStart, SuffixBuffer, SuffixLength * sizeof(WCHAR)) == 0);
            } else {
                LPCWSTR SourceEnd = SourceBuffer + SourceLength;
                SourceBuffer += nSourceStart;
                while (SourceBuffer < SourceEnd) {
                    WCHAR srcChar = *SourceBuffer;
                    if (srcChar>='A' && srcChar<='Z') {
                        srcChar|=0x20;
                    }
                    WCHAR suffixChar = *SuffixBuffer;
                    if (suffixChar>='A' && suffixChar<='Z') {
                        suffixChar|=0x20;
                    }
                    if (srcChar!=suffixChar) {
                        return (FALSE);
                    }
                    SourceBuffer++; SuffixBuffer++;
                }
                return (TRUE);
            }
        }
    }
    
    return ((NativeCompareInfo*)pNativeCompareInfo)->IsSuffix(SourceBuffer, SourceLength, SuffixBuffer, SuffixLength, dwFlags);
}
FCIMPLEND

////////////////////////////////////////////////////////////////////////////
//
//  LastIndexOfString
//
////////////////////////////////////////////////////////////////////////////

FCIMPL7(INT32, COMNlsInfo::LastIndexOfString, INT_PTR pNativeCompareInfo, INT32 myLCID, StringObject* pString1UNSAFE, StringObject* pString2UNSAFE, INT32 StartIndex, INT32 Count, INT32 dwFlags)
{
    THROWSCOMPLUSEXCEPTION();

    INT32 iRetVal = -1;
    STRINGREF pString1 = (STRINGREF) pString1UNSAFE;
    STRINGREF pString2 = (STRINGREF) pString2UNSAFE;
    HELPER_METHOD_FRAME_BEGIN_RET_2(pString1, pString2);
    //
    //  Make sure there are strings.
    //
    if ((pString1 == NULL) || (pString2 == NULL))
    {
        COMPlusThrowArgumentNull((pString1 == NULL ? L"string1": L"string2"),L"ArgumentNull_String");
    }

    //
    //  Get the arguments.
    //
    int startIndex = StartIndex;
    int count = Count;
    int stringLength1 = pString1->GetStringLength();
    int findLength = pString2->GetStringLength();

    WCHAR *buffString = NULL;
    WCHAR *buffFind = NULL;

    //
    //  Check the ranges.
    //
    if (stringLength1 == 0)
    {
        if (findLength == 0) {
            iRetVal = 0;
            goto lExit;
        }
        goto lExit;
    }

    if ((startIndex < 0) || (startIndex > stringLength1))
    {
        COMPlusThrowArgumentOutOfRange(L"startIndex", L"ArgumentOutOfRange_Index");
    }

    int endIndex;
    if (count == -1)
    {
        endIndex = 0;
    }
    else
    {
        if ((count < 0) || (count - 1 > startIndex))
        {
            COMPlusThrowArgumentOutOfRange(L"count", L"ArgumentOutOfRange_Count");
        }
        endIndex = startIndex - count + 1;
    }

    //
    //  See if we have an empty string 2.
    //
    if (findLength == 0)
    {
        iRetVal = startIndex;
        goto lExit;
    }

    //
    //  Search for the character in the string.
    buffString = pString1->GetBuffer();
    buffFind = pString2->GetBuffer();

    if (dwFlags == COMPARE_OPTIONS_ORDINAL) {
        iRetVal = FastLastIndexOfString(buffString, startIndex, endIndex, buffFind, findLength);
        goto lExit;
    }

    //For dwFlags, 0 is the default, 1 is ignore case, we can handle both.
    if (dwFlags<=1 && IS_FAST_COMPARE_LOCALE(myLCID)) {
        //If we've never before looked at whether this string has high chars, do so now.
        if (IS_STRING_STATE_UNDETERMINED(pString1->GetHighCharState())) {
            COMString::InternalCheckHighChars(pString1);
        }
        
        //If we've never before looked at whether this string has high chars, do so now.
        if (IS_STRING_STATE_UNDETERMINED(pString2->GetHighCharState())) {
            COMString::InternalCheckHighChars(pString2);
        }

        //If neither string has high chars, we can use a much faster comparison algorithm.
        if (IS_FAST_INDEX(pString1->GetHighCharState()) && IS_FAST_INDEX(pString2->GetHighCharState())) {
            if (dwFlags==0) {
                iRetVal = FastLastIndexOfString(buffString, startIndex, endIndex, buffFind, findLength);
                goto lExit;
            } else {
                iRetVal = FastLastIndexOfStringInsensitive(buffString, startIndex, endIndex, buffFind, findLength);
                goto lExit;
            }
        }
    }

    int nMatchEndIndex;
    iRetVal = ((NativeCompareInfo*)(pNativeCompareInfo))->LastIndexOfString(
        buffString, buffFind, startIndex, endIndex, findLength, dwFlags, &nMatchEndIndex);
    if (iRetVal == INDEXOF_INVALID_FLAGS) {
        COMPlusThrowArgumentException(L"flags", L"Argument_InvalidFlag");
    }
lExit: ;
    HELPER_METHOD_FRAME_END();
    return iRetVal;
}
FCIMPLEND


////////////////////////////////////////////////////////////////////////////
//
//  nativeCreateSortKey
//
////////////////////////////////////////////////////////////////////////////

FCIMPL5(Object*, COMNlsInfo::nativeCreateSortKey, Object* pThisUNSAFE, INT_PTR pNativeCompareInfo, StringObject* pStringUNSAFE, INT32 dwFlags, INT32 SortId)
{
    THROWSCOMPLUSEXCEPTION();

    OBJECTREF   pThis       = (OBJECTREF) pThisUNSAFE;
    STRINGREF   pString     = (STRINGREF) pStringUNSAFE;
    U1ARRAYREF  ResultArray = NULL;

    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_2(Frame::FRAME_ATTR_RETURNOBJ, pThis, pString);

    //
    //  Make sure there is a string.
    //
    if (!pString) {
        COMPlusThrowArgumentNull(L"string",L"ArgumentNull_String");
    }

    WCHAR* wstr;
    int Length;
    DWORD dwFlags = (LCMAP_SORTKEY | dwFlags);
    int ByteCount = 0;
    LPBYTE pByte = NULL;

    Length = pString->GetStringLength();

    if (Length==0) {
        //If they gave us an empty string, we're going to create an empty array.
        //This will serve to be less than any other compare string when we call sortkey_compare.
        ResultArray = (U1ARRAYREF)AllocatePrimitiveArray(ELEMENT_TYPE_U1,0);
        goto lExit;
    }

    //This just gets the correct size for the table.
    ByteCount = ((NativeCompareInfo*)(pNativeCompareInfo))->MapSortKey(dwFlags, (wstr = pString->GetBuffer()), Length, NULL, 0);

    //A count of 0 indicates that we either had an error or had a zero length string originally.
    if (ByteCount==0) {
        COMPlusThrow(kArgumentException, L"Argument_MustBeString");
    }


    ResultArray = (U1ARRAYREF)AllocatePrimitiveArray(ELEMENT_TYPE_U1, ByteCount);

    //The previous allocation could have caused the buffer to move.
    wstr = pString->GetBuffer();

    pByte = (LPBYTE)(ResultArray->GetDirectPointerToNonObjectElements());

    //MapSortKey doesn't do anything that could cause GC to occur.
#ifdef _DEBUG
    _ASSERTE(((NativeCompareInfo*)(pNativeCompareInfo))->MapSortKey(dwFlags, wstr, Length, pByte, ByteCount) != 0);
#else
    ((NativeCompareInfo*)(pNativeCompareInfo))->MapSortKey(dwFlags, wstr, Length, pByte, ByteCount);
#endif

lExit: ;
    HELPER_METHOD_FRAME_END();

    return OBJECTREFToObject(ResultArray);

}
FCIMPLEND

FCIMPL1(INT32, COMNlsInfo::nativeGetTwoDigitYearMax, INT32 nValue)
{
    DWORD dwTwoDigitYearMax = (DWORD) -1;
    THROWSCOMPLUSEXCEPTION();

    HELPER_METHOD_FRAME_BEGIN_RET_0();

    if (GetCalendarInfo(
                    LOCALE_USER_DEFAULT,
                    nValue,
                    CAL_ITWODIGITYEARMAX | CAL_RETURN_NUMBER,
                    NULL,
                    0,
                    &dwTwoDigitYearMax) == 0) {
        dwTwoDigitYearMax = (DWORD) -1;
        goto lExit;
    }

lExit: ;
    HELPER_METHOD_FRAME_END();

    return (dwTwoDigitYearMax);
}
FCIMPLEND

FCIMPL0(LONG, COMNlsInfo::nativeGetTimeZoneMinuteOffset)
{
    TIME_ZONE_INFORMATION timeZoneInfo;

    GetTimeZoneInformation(&timeZoneInfo);

    //
    // In Win32, UTC = local + offset.  So for Pacific Standard Time, offset = 8.
    // In NLS+, Local time = UTC + offset. So for PST, offset = -8.
    // So we have to reverse the sign here.
    //
    return (timeZoneInfo.Bias * -1);
}
FCIMPLEND

FCIMPL0(Object*, COMNlsInfo::nativeGetStandardName)
{
    STRINGREF refRetVal = NULL;
    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, refRetVal);

    TIME_ZONE_INFORMATION timeZoneInfo;
    GetTimeZoneInformation(&timeZoneInfo);
    
    refRetVal = COMString::NewString(timeZoneInfo.StandardName);

    HELPER_METHOD_FRAME_END();
    return OBJECTREFToObject(refRetVal);
}
FCIMPLEND

FCIMPL0(Object*, COMNlsInfo::nativeGetDaylightName)
{
    STRINGREF refRetVal = NULL;
    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, refRetVal);

    TIME_ZONE_INFORMATION timeZoneInfo;
    GetTimeZoneInformation(&timeZoneInfo);
    // Instead of returning null when daylight saving is not used, now we return the same result as the OS.  
    //In this case, if daylight saving time is used, the standard name is returned.


    refRetVal = COMString::NewString(timeZoneInfo.DaylightName);

    HELPER_METHOD_FRAME_END();
    return OBJECTREFToObject(refRetVal);
}
FCIMPLEND

FCIMPL0(Object*, COMNlsInfo::nativeGetDaylightChanges)
{
    I2ARRAYREF pResultArray = NULL;
    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, pResultArray);

    TIME_ZONE_INFORMATION timeZoneInfo;
    DWORD result =  GetTimeZoneInformation(&timeZoneInfo);

    if (result == TIME_ZONE_ID_UNKNOWN || timeZoneInfo.DaylightBias == 0 
        ) {
        // If daylight saving time is not used in this timezone, return null.
        //
        // Windows NT/2000: TIME_ZONE_ID_UNKNOWN is returned if daylight saving time is not used in
        // the current time zone, because there are no transition dates.
        // If the current timezone uses daylight saving rule, but user unchekced the
        // "Automatically adjust clock for daylight saving changes", the value 
        // for DaylightBias will be 0.
        goto lExit;
    }

    pResultArray = (I2ARRAYREF)AllocatePrimitiveArray(ELEMENT_TYPE_I2, 17);

    //
    // The content of timeZoneInfo.StandardDate is 8 words, which
    // contains year, month, day, dayOfWeek, hour, minute, second, millisecond.
    //
    memcpy(pResultArray->m_Array,
            (LPVOID)&timeZoneInfo.DaylightDate,
            8 * sizeof(INT16));   

    //
    // The content of timeZoneInfo.DaylightDate is 8 words, which
    // contains year, month, day, dayOfWeek, hour, minute, second, millisecond.
    //
    memcpy(((INT16*)pResultArray->m_Array) + 8,
            (LPVOID)&timeZoneInfo.StandardDate,
            8 * sizeof(INT16));

    ((INT16*)pResultArray->m_Array)[16] = (INT16)timeZoneInfo.DaylightBias * -1;

lExit: ;
    HELPER_METHOD_FRAME_END();
    return OBJECTREFToObject(pResultArray);
}
FCIMPLEND

////////////////////////////////////////////////////////////////////////////
//
//  SortKey_Compare
//
////////////////////////////////////////////////////////////////////////////

FCIMPL2(INT32, COMNlsInfo::SortKey_Compare, Object* pSortKey1UNSAFE, Object* pSortKey2UNSAFE)
{
    THROWSCOMPLUSEXCEPTION();

    INT32     iRetVal   = 0;
    OBJECTREF pSortKey1 = (OBJECTREF) pSortKey1UNSAFE;
    OBJECTREF pSortKey2 = (OBJECTREF) pSortKey2UNSAFE;
    HELPER_METHOD_FRAME_BEGIN_RET_2(pSortKey1, pSortKey2);

    if ((pSortKey1 == NULL) || (pSortKey2 == NULL))
    {
        COMPlusThrowArgumentNull((pSortKey1 == NULL ? L"sortKey1": L"sortKey2"),L"ArgumentNull_String");
    }
    U1ARRAYREF pDataRef1 = internalGetField<U1ARRAYREF>( pSortKey1,
                                                         "m_KeyData",
                                                         &gsig_Fld_ArrByte );
    BYTE* Data1 = (BYTE*)pDataRef1->m_Array;

    U1ARRAYREF pDataRef2 = internalGetField<U1ARRAYREF>( pSortKey2,
                                                         "m_KeyData",
                                                         &gsig_Fld_ArrByte );
    BYTE* Data2 = (BYTE*)pDataRef2->m_Array;

    int Length1 = pDataRef1->GetNumComponents();
    int Length2 = pDataRef2->GetNumComponents();

    for (int ctr = 0; (ctr < Length1) && (ctr < Length2); ctr++)
    {
        if (Data1[ctr] > Data2[ctr])
        {
            iRetVal = 1;
        }
        else if (Data1[ctr] < Data2[ctr])
        {
            iRetVal = -1;
        }
    }

    HELPER_METHOD_FRAME_END();
    return iRetVal;
}
FCIMPLEND

FCIMPL4(INT32, COMNlsInfo::nativeChangeCaseChar, 
    INT32 nLCID, INT_PTR pNativeTextInfo, WCHAR wch, BYTE bIsToUpper) {
    //
    // If our character is less than 0x80 and we're in US English locale, we can make certain
    // assumptions that allow us to do this a lot faster.  US English is 0x0409.  The "Invariant
    // Locale" is 0x0.
    //
    if ((nLCID == 0x0409 || nLCID==0x0) && wch < 0x7f) {
        return (bIsToUpper ? ToUpperMapping[wch] : ToLowerMapping[wch]);
    }

    NativeTextInfo* pNativeTextInfoPtr = (NativeTextInfo*)pNativeTextInfo;
    return (pNativeTextInfoPtr->ChangeCaseChar(bIsToUpper, wch));
}
FCIMPLEND

FCIMPL4(Object*, COMNlsInfo::nativeChangeCaseString, INT32 nLCID, INT_PTR pNativeTextInfo, StringObject* pStringUNSAFE, BYTE bIsToUpper)
{
    STRINGREF pResult = NULL;
    STRINGREF pString = ObjectToSTRINGREF(pStringUNSAFE);

    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_2(Frame::FRAME_ATTR_RETURNOBJ, pString, pResult)

    //
    //  Get the length of the string.
    //
    int nLength = pString->GetStringLength();

    //
    //  Check if we have the empty string.
    //
    if (nLength == 0) 
    {
        pResult = ObjectToSTRINGREF(pString);
    }
    else
    {
    //
    //  Create the result string.
    //
        pResult = COMString::NewString(nLength);
    LPWSTR pResultStr = pResult->GetBuffer();

    //
    // If we've never before looked at whether this string has high chars, do so now.
    //
        if (IS_STRING_STATE_UNDETERMINED(pString->GetHighCharState())) 
        {
            COMString::InternalCheckHighChars(pString);
    }

    //
    // If all of our characters are less than 0x80 and in a USEnglish or Invariant Locale, we can make certain
    // assumptions that allow us to do this a lot faster.
    //

        if ((nLCID == 0x0409 || nLCID==0x0) && IS_FAST_CASING(pString->GetHighCharState())) 
        {
            ConvertStringCaseFast(pString->GetBuffer(), pResultStr, nLength, bIsToUpper ? LCMAP_UPPERCASE : LCMAP_LOWERCASE);
        pResult->ResetHighCharState();
        } 
        else 
        {
            NativeTextInfo* pNativeTextInfoPtr = (NativeTextInfo*)pNativeTextInfo;
            pNativeTextInfoPtr->ChangeCaseString(bIsToUpper, nLength, pString->GetBuffer(), pResultStr);
    }            
    pResult->SetStringLength(nLength);
    pResultStr[nLength] = 0;
    }

    HELPER_METHOD_FRAME_END();
    
    return OBJECTREFToObject(pResult);
}
FCIMPLEND

FCIMPL2(INT32, COMNlsInfo::nativeGetTitleCaseChar, 
    INT_PTR pNativeTextInfo, WCHAR wch) {
    NativeTextInfo* pNativeTextInfoPtr = (NativeTextInfo*)pNativeTextInfo;
    return (pNativeTextInfoPtr->GetTitleCaseChar(wch));
}    
FCIMPLEND


/*==========================AllocateDefaultCasingTable==========================
**Action:  A thin wrapper for the casing table functionality.
**Returns:
**Arguments:
**Exceptions:
==============================================================================*/
FCIMPL0(LPVOID, COMNlsInfo::AllocateDefaultCasingTable)
{
    LPVOID rv;

    HELPER_METHOD_FRAME_BEGIN_RET_0();

    THROWSCOMPLUSEXCEPTION();
    // This method is not thread-safe.  It needs managed code to provide synchronization.
    // The code is in the static ctor of TextInfo.
    if (m_pCasingTable == NULL) {
        m_pCasingTable = new CasingTable();
    }
    if (m_pCasingTable == NULL) {
        COMPlusThrowOM();
    }
    if (!m_pCasingTable->AllocateDefaultTable()) {
        COMPlusThrowOM();
    }

    rv = (LPVOID)m_pCasingTable->GetDefaultNativeTextInfo();

    HELPER_METHOD_FRAME_END();

    return rv;
}
FCIMPLEND

/*=============================AllocateCasingTable==============================
**Action: This is a thin wrapper for the CasingTable that allows us not to have 
**        additional .h files.
**Returns:
**Arguments:
**Exceptions:
==============================================================================*/

FCIMPL1(LPVOID, COMNlsInfo::AllocateCasingTable, INT32 lcid) 
{
    THROWSCOMPLUSEXCEPTION();

    NativeTextInfo* pNativeTextInfo;
    
    HELPER_METHOD_FRAME_BEGIN_RET_0();

    pNativeTextInfo = m_pCasingTable->InitializeNativeTextInfo(lcid);

    if (pNativeTextInfo == NULL)
    {
        COMPlusThrowOM();
    }

    HELPER_METHOD_FRAME_END();

    RETURN(pNativeTextInfo, LPVOID);
}
FCIMPLEND

/*================================GetCaseInsHash================================
**Action:
**Returns:
**Arguments:
**Exceptions:
==============================================================================*/
FCIMPL2(INT32, COMNlsInfo::GetCaseInsHash, LPVOID pvStrA, void *pNativeTextInfoPtr) {

    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(pvStrA!=NULL);
    STRINGREF strA;
    INT32 highCharState;
    INT32 result;
    
    *((LPVOID *)&strA)=pvStrA;
    LPCWSTR pCurrStr = (LPCWSTR)strA->GetBuffer();

    //
    // Check the high-char state of the string.  Mark this on the String for
    // future use.
    //
    if (IS_STRING_STATE_UNDETERMINED(highCharState=strA->GetHighCharState())) {
        COMString::InternalCheckHighChars(strA);
        highCharState=strA->GetHighCharState();
    }

    //
    // If we know that we don't have any high characters (the common case) we can
    // call a hash function that knows how to do a very fast case conversion.  If
    // we find characters above 0x80, it's much faster to convert the entire string
    // to Uppercase and then call the standard hash function on it.
    //
    if (IS_FAST_CASING((UINT32) highCharState)) {
        result = HashiStringKnownLower80(pCurrStr);
    } else {
        CQuickBytes newBuffer;
        INT32 length = strA->GetStringLength();
        WCHAR *pNewStr = (WCHAR *)newBuffer.Alloc((length + 1) * sizeof(WCHAR));
        if (!pNewStr) {
            FCThrow(kOutOfMemoryException);
        }
        ((NativeTextInfo*)pNativeTextInfoPtr)->ChangeCaseString(true, length, (LPWSTR)pCurrStr, pNewStr);
        pNewStr[length]='\0';
        result = HashString(pNewStr);
    }
    return result;
}
FCIMPLEND

/**
 * This function returns a pointer to this table that we use in System.Globalization.EncodingTable.
 * No error checking of any sort is performed.  Range checking is entirely the responsiblity of the managed
 * code.
 */
FCIMPL0(EncodingDataItem *, COMNlsInfo::nativeGetEncodingTableDataPointer) {
    return (EncodingDataItem *)EncodingDataTable;
}
FCIMPLEND

/**
 * This function returns a pointer to this table that we use in System.Globalization.EncodingTable.
 * No error checking of any sort is performed.  Range checking is entirely the responsiblity of the managed
 * code.
 */
FCIMPL0(CodePageDataItem *, COMNlsInfo::nativeGetCodePageTableDataPointer) {
    return ((CodePageDataItem*) CodePageDataTable);
}
FCIMPLEND

//
// Casing Table Helpers for use in the EE.
//

#define TEXTINFO_CLASSNAME "System.Globalization.TextInfo"

NativeTextInfo *InternalCasingHelper::pNativeTextInfo=NULL;
void InternalCasingHelper::InitTable() {
    EEClass *pClass;
    MethodTable *pMT;
    BOOL bResult;
    
    if (!pNativeTextInfo) {
        pClass = SystemDomain::Loader()->LoadClass(TEXTINFO_CLASSNAME);
        _ASSERTE(pClass!=NULL || "Unable to load the class for TextInfo.  This is required for CaseInsensitive Type Lookups.");

        pMT = pClass->GetMethodTable();
        _ASSERTE(pMT!=NULL || "Unable to load the MethodTable for TextInfo.  This is required for CaseInsensitive Type Lookups.");

        bResult = pMT->CheckRunClassInit(NULL);
        _ASSERTE(bResult || "CheckRunClassInit Failed on TextInfo.");
        
        pNativeTextInfo = COMNlsInfo::m_pCasingTable->GetDefaultNativeTextInfo();
        _ASSERTE(pNativeTextInfo || "Unable to get a casing table for 0x0409.");
    }

    _ASSERTE(pNativeTextInfo || "Somebody has nulled the casing table required for case-insensitive type lookups.");

}

INT32 InternalCasingHelper::InvariantToLower(LPUTF8 szOut, int cMaxBytes, LPCUTF8 szIn) {
    _ASSERTE(szOut);
    _ASSERTE(szIn);

    //Figure out the maximum number of bytes which we can copy without
    //running out of buffer.  If cMaxBytes is less than inLength, copy
    //one fewer chars so that we have room for the null at the end;
    int inLength = (int)(strlen(szIn)+1);
    int maxCopyLen  = (inLength<=cMaxBytes)?inLength:(cMaxBytes-1);
    LPUTF8 szEnd;
    LPCUTF8 szInSave = szIn;
    LPUTF8 szOutSave = szOut;
    BOOL bFoundHighChars=FALSE;

    //Compute our end point.
    szEnd = szOut + maxCopyLen;

    //Walk the string copying the characters.  Change the case on
    //any character between A-Z.
    for (; szOut<szEnd; szOut++, szIn++) {
        if (*szIn>='A' && *szIn<='Z') {
            *szOut = *szIn | 0x20;
        } else {
            if (((UINT32)(*szIn))>((UINT32)0x80)) {
                bFoundHighChars = TRUE;
                break;
            }
            *szOut = *szIn;
        }
    }

    if (!bFoundHighChars) {
        //If we copied everything, tell them how many bytes we copied, 
        //and arrange it so that the original position of the string + the returned
        //length gives us the position of the null (useful if we're appending).
        if (maxCopyLen==inLength) {
            return maxCopyLen-1;
        } else {
            *szOut=0;
            return (maxCopyLen - inLength);
        }
    }

    InitTable();
    szOut = szOutSave;
    CQuickBytes qbOut;
    LPWSTR szWideOut;

    //convert the UTF8 to Unicode
    MAKE_WIDEPTR_FROMUTF8(szInWide, szInSave);
    INT32 wideCopyLen = (INT32)wcslen(szInWide);
    szWideOut = (LPWSTR)qbOut.Alloc((wideCopyLen + 1) * sizeof(WCHAR));

    //Do the casing operation
    pNativeTextInfo->ChangeCaseString(FALSE/*IsToUpper*/, wideCopyLen, szInWide, szWideOut);    
    szWideOut[wideCopyLen]=0;

    //Convert the Unicode back to UTF8
    INT32 result = WszWideCharToMultiByte(CP_UTF8, 0, szWideOut, wideCopyLen, szOut, cMaxBytes, NULL, NULL);
    _ASSERTE(result!=0);
    szOut[result]=0;//Null terminate the result string.

    //Setup the return value.
    if (result>0 && (inLength==maxCopyLen)) {
        //This is the success case.
        return (result-1);
    } else {
        //We didn't have enough space.  Specify how much we're going to need.
        result = WszWideCharToMultiByte(CP_UTF8, 0, szWideOut, wideCopyLen, NULL, 0, NULL, NULL);
        return (cMaxBytes - result);
    }
}


//
// This table should be sorted using case-insensitive ordinal order.
// In the managed code, String.CompareStringOrdinalWC() is used to sort this.


/**
 * This function returns the number of items in EncodingDataTable.
 */
FCIMPL0(INT32, COMNlsInfo::nativeGetNumEncodingItems) {
    return (m_nEncodingDataTableItems);
}
FCIMPLEND
    
const WCHAR COMNlsInfo::ToUpperMapping[] = {0x0,0x1,0x2,0x3,0x4,0x5,0x6,0x7,0x8,0x9,0xa,0xb,0xc,0xd,0xe,0xf,
                                            0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,
                                            0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,
                                            0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,
                                            0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x4b,0x4c,0x4d,0x4e,0x4f,
                                            0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a,0x5b,0x5c,0x5d,0x5e,0x5f,
                                            0x60,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x4b,0x4c,0x4d,0x4e,0x4f,
                                            0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a,0x7b,0x7c,0x7d,0x7e,0x7f};

const WCHAR COMNlsInfo::ToLowerMapping[] = {0x0,0x1,0x2,0x3,0x4,0x5,0x6,0x7,0x8,0x9,0xa,0xb,0xc,0xd,0xe,0xf,
                                            0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,
                                            0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,
                                            0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,
                                            0x40,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a,0x6b,0x6c,0x6d,0x6e,0x6f,
                                            0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x5b,0x5c,0x5d,0x5e,0x5f,
                                            0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a,0x6b,0x6c,0x6d,0x6e,0x6f,
                                            0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x7b,0x7c,0x7d,0x7e,0x7f};


const INT8 COMNlsInfo::ComparisonTable[0x80][0x80] = {
{ 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
{ 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
{ 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
{ 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
{ 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
{ 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
{ 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
{ 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1, 1,-1,-1,-1,-1,-1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1, 1,-1,-1,-1,-1,-1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1, 1,-1,-1,-1,-1,-1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1, 1,-1,-1,-1,-1,-1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1, 1,-1,-1,-1,-1,-1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1, 1,-1,-1,-1,-1,-1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1, 1,-1,-1,-1,-1,-1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1, 1,-1,-1,-1,-1,-1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1, 1,-1,-1,-1,-1,-1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1, 1,-1,-1,-1,-1,-1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1, 1,-1,-1,-1,-1,-1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1,-1,-1,-1,-1,-1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1,-1,-1,-1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1, 0, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1, 1,-1,-1,-1,-1,-1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 0,-1,-1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 0,-1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 0, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1,-1,-1,-1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1,-1,-1,-1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1,-1,-1,-1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1,-1,-1,-1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1,-1,-1,-1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1,-1,-1,-1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1,-1,-1,-1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1,-1,-1,-1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1,-1,-1,-1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 0,-1,-1,-1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1,-1,-1,-1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 0,-1,-1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1,-1,-1,-1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 0,-1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1,-1,-1,-1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 0, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 0}
};    

