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
//  Purpose:  This module defines the methods of the COMNlsInfo
//            class.  These methods are the helper functions for the
//            managed NLS+ classes.
//
//  Date:     August 12, 1998
//
////////////////////////////////////////////////////////////////////////////

#ifndef _COMNLSINFO_H
#define _COMNLSINFO_H


//
//This structure must map 1-for-1 with the InternalDataItem structure in 
//System.Globalization.EncodingTable.
//
struct EncodingDataItem {
    WCHAR* webName;
    int    codePage;
};

//
//This structure must map 1-for-1 with the InternalCodePageDataItem structure in 
//System.Globalization.EncodingTable.
//
struct CodePageDataItem {
    int    codePage;
    int    uiFamilyCodePage;
    WCHAR* webName;
    WCHAR* headerName;
    WCHAR* bodyName;
    DWORD dwFlags;
};

//
//This structure must map 1-for-1 with the NameOffsetItem structure in 
//System.Globalization.CultureTable.
//
struct NameOffsetItem {
	WORD 	strOffset;		// Offset (in words) to a string in the String Pool Table.
	WORD	dataItemIndex;	// Index to a record in Culture Data Table.
};

//
// This is the header for BaseInfoTable/CultureInfoTable/RegionInfoTable
//
//
//This structure must map 1-for-1 with the CultureInfoHeader structure in 
//System.Globalization.CultureTable.
//
struct CultureInfoHeader {
	DWORD 	version;		// version
	WORD	hashID[8];		// 128 bit hash ID
	WORD	numCultures;	// Number of cultures
	WORD 	maxPrimaryLang;	// Max number of primary language
	WORD	numWordFields;	// Number of WORD value fields.
	WORD	numStrFields;	// Number of string value fields.
	WORD    numWordRegistry;    // Number of registry entries for WORD values.
	WORD    numStringRegistry;  // Number of registry entries for String values.
	DWORD   wordRegistryOffset; // Offset (in bytes) to WORD Registry Offset Table.
	DWORD   stringRegistryOffset;   // Offset (in bytes) to String Registry Offset Table.
	DWORD	IDTableOffset;	// Offset (in bytes) to Culture ID Offset Table.
	DWORD	nameTableOffset;// Offset (in bytes) to Name Offset Table.
	DWORD	dataTableOffset;// Offset (in bytes) to Culture Data Table.
	DWORD	stringPoolOffset;// Offset (in bytes) to String Pool Table.
};


////////////////////////////////////////////////////////////////////////////
//
// Forward declarations
//
////////////////////////////////////////////////////////////////////////////

class CharTypeTable;
class CasingTable;
class SortingTable;

class COMNlsInfo {
public:
    static BOOL InitializeNLS();


private:

    //
    // NOTENOTE
    // WHEN ECALL IS USED, THE PARAMETERS IN THE STRUCTURE
    // SHOULD BE DEFINED IN REVERSED ORDER.
    //
    //struct VoidArgs
    //{
    //};
    //struct Int32Arg
    //{
    //    DECLARE_ECALL_I4_ARG(INT32, nValue);
    //};
    //struct Int32Int32Arg
    //{
    //    DECLARE_ECALL_I4_ARG(INT32, nValue2);
    //    DECLARE_ECALL_I4_ARG(INT32, nValue1);
    //};
    //struct CultureInfo_GetCultureInfoArgs3
    //{
    //    DECLARE_ECALL_I4_ARG(BOOL, UseUserOverride);
    //    DECLARE_ECALL_I4_ARG(INT32, ValueField);
    //    DECLARE_ECALL_I4_ARG(INT32, CultureDataItem);
    //};
    //struct CompareInfo_IndexOfCharArgs
    //{
    //    DECLARE_ECALL_I4_ARG(INT32, dwFlags);
    //    DECLARE_ECALL_I4_ARG(INT32, Count);
    //    DECLARE_ECALL_I4_ARG(INT32, StartIndex);
    //    DECLARE_ECALL_OBJECTREF_ARG(WCHAR, ch);
    //    DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, pString);
    //    DECLARE_ECALL_I4_ARG(INT32, LCID);
    //    DECLARE_ECALL_I4_ARG(INT_PTR, pNativeCompareInfo);
    //};
    //struct CompareInfo_IndexOfStringArgs
    //{
    //    DECLARE_ECALL_I4_ARG(INT32, dwFlags);
    //    DECLARE_ECALL_I4_ARG(INT32, Count);
    //    DECLARE_ECALL_I4_ARG(INT32, StartIndex);
    //    DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, pString2);
    //    DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, pString1);
    //    DECLARE_ECALL_I4_ARG(INT32, LCID);
    //    DECLARE_ECALL_I4_ARG(INT_PTR, pNativeCompareInfo);
    //};
    //struct SortKey_CreateSortKeyArgs
    //{
    //    DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, pThis);
    //    DECLARE_ECALL_I4_ARG(INT32, SortId);
    //    DECLARE_ECALL_I4_ARG(INT32, dwFlags);
    //    DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, pString);
    //    DECLARE_ECALL_I4_ARG(INT_PTR, pNativeCompareInfo);
    //};
    //struct SortKey_CompareArgs
    //{
    //    DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, pSortKey2);
    //    DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, pSortKey1);
    //};

public:

    //
    //  Native helper functions for methods in CultureInfo.
    //
    static FCDECL1(INT32, IsSupportedLCID, INT32);
    static FCDECL1(INT32, IsInstalledLCID, INT32);

    static FCDECL0(INT32, nativeGetUserDefaultLCID);
    
 	static FCDECL0(INT32, nativeGetUserDefaultUILanguage);
 	static FCDECL0(INT32, nativeGetSystemDefaultUILanguage);

    //
    // Native helper functions for methods in DateTimeFormatInfo
    //
    static FCDECL2(INT32, GetCaseInsHash, LPVOID strA, void *pNativeTextInfoPtr);

    static FCDECL0(VOID, nativeInitCultureInfoTable);
    static FCDECL0(CultureInfoHeader*, nativeGetCultureInfoHeader);
    static FCDECL1(LPWSTR,  nativeGetCultureInfoStringPoolStr, INT32);
    static FCDECL0(NameOffsetItem*, nativeGetCultureInfoNameOffsetTable);
    
    static FCDECL1(INT32, nativeGetCultureDataFromID, INT32);
    static FCDECL3(INT32, GetCultureInt32Value, INT32, INT32, BYTE);
    static FCDECL2(INT32, GetCultureDefaultInt32Value, INT32, INT32);
    
    static FCDECL3(LPVOID, GetCultureStringValue, INT32 CultureDataItem, INT32 ValueField, BYTE UseUserOverride);
    static FCDECL2(LPVOID,  GetCultureDefaultStringValue, INT32 CultureDataItem, INT32 ValueField);
    static FCDECL3(Object*, GetCultureMultiStringValues, INT32 CultureDataItem, INT32 ValueField, BYTE UseUserOverride);

    static FCDECL0(VOID, nativeInitRegionInfoTable);
    static FCDECL0(CultureInfoHeader*, nativeGetRegionInfoHeader);
    static FCDECL1(LPWSTR,  nativeGetRegionInfoStringPoolStr, INT32);
    static FCDECL0(NameOffsetItem*, nativeGetRegionInfoNameOffsetTable);
    
    static FCDECL1(INT32, nativeGetRegionDataFromID, INT32);
    static FCDECL3(INT32, nativeGetRegionInt32Value, INT32 CultureDataItem, INT32 ValueField, BYTE UseUserOverride);
    static FCDECL3(Object*, nativeGetRegionStringValue, INT32 CultureDataItem, INT32 ValueField, BYTE UseUserOverride);

    static FCDECL0(void,    nativeInitCalendarTable);
    static FCDECL3(INT32,   nativeGetCalendarInt32Value, INT32 CultureDataItem, INT32 ValueField, BYTE UseUserOverride);
    static FCDECL3(Object*, nativeGetCalendarStringValue, INT32 CultureDataItem, INT32 ValueField, BYTE UseUserOverride);
    static FCDECL3(Object*, nativeGetCalendarMultiStringValues, INT32 CultureDataItem, INT32 ValueField, BYTE UseUserOverride);
    static FCDECL2(Object*, nativeGetEraName, INT32 nValue1, INT32 nValue2);
    
    static FCDECL0(CultureInfoHeader*, nativeGetCalendarHeader);
    static FCDECL1(LPWSTR,  nativeGetCalendarStringPoolStr, INT32);

    static FCDECL0(VOID,   nativeInitUnicodeCatTable);
    static FCDECL0(LPVOID, nativeGetUnicodeCatTable);
    static FCDECL0(LPVOID, nativeGetUnicodeCatLevel2Offset);
    static BYTE GetUnicodeCategory(WCHAR wch);
    static BOOL nativeIsWhiteSpace(WCHAR c);

    static FCDECL0(LPVOID, nativeGetNumericTable);
    static FCDECL0(LPVOID, nativeGetNumericLevel2Offset);
    static FCDECL0(LPVOID, nativeGetNumericFloatData);
    
    static FCDECL0(INT32, nativeGetThreadLocale);
    static FCDECL1(BOOL,  nativeSetThreadLocale, INT32 lcid);

    //
    //  Native helper functions for methods in CompareInfo.
    //
    static FCDECL5(INT32, Compare,           INT_PTR pNativeCompareInfo, INT32 LCID, StringObject* pString1UNSAFE, StringObject* pString2UNSAFE, INT32 dwFlags);
    static FCDECL9(INT32, CompareRegion,     INT_PTR pNativeCompareInfo, INT32 LCID, StringObject* pString1, INT32 Offset1, INT32 Length1, StringObject* pString2, INT32 Offset2, INT32 Length2, INT32 dwFlags);
    static FCDECL7(INT32, IndexOfChar,       INT_PTR pNativeCompareInfo, INT32 LCID, StringObject* pStringUNSAFE,  WCHAR ch, INT32 StartIndex, INT32 Count, INT32 dwFlags);
    static FCDECL7(INT32, IndexOfString,     INT_PTR pNativeCompareInfo, INT32 LCID, StringObject* pString1UNSAFE, StringObject* pString2UNSAFE, INT32 StartIndex, INT32 Count, INT32 dwFlags);
    static FCDECL7(INT32, LastIndexOfChar,   INT_PTR pNativeCompareInfo, INT32 LCID, StringObject* pStringUNSAFE,  WCHAR ch, INT32 StartIndex, INT32 Count, INT32 dwFlags);
    static FCDECL7(INT32, LastIndexOfString, INT_PTR pNativeCompareInfo, INT32 LCID, StringObject* pString1UNSAFE, StringObject* pString2UNSAFE, INT32 StartIndex, INT32 Count, INT32 dwFlags);
    static FCDECL5(INT32, nativeIsPrefix,    INT_PTR pNativeCompareInfo, INT32 LCID, StringObject* pString1, StringObject* pString2, INT32 dwFlags);
    static FCDECL5(INT32, nativeIsSuffix,    INT_PTR pNativeCompareInfo, INT32 LCID, StringObject* pString1, StringObject* pString2, INT32 dwFlags);
    
    //
    //  Native helper functions for methods in SortKey.
    //
    static FCDECL5(Object*, nativeCreateSortKey, Object* pThisUNSAFE, INT_PTR pNativeCompareInfo, StringObject* pStringUNSAFE, INT32 dwFlags, INT32 SortId);
    static FCDECL2(INT32, SortKey_Compare, Object* pSortKey1UNSAFE, Object* pSortKey2UNSAFE);

	//
	//  Native helper functions for methods in NLSDataTable
	//
    //	static INT32 __stdcall GetLCIDFromCultureName(NLSDataTable_GetLCIDFromCultureNameArgs* pargs);

    //
    //  Native helper function for methods in Calendar
    //
    static FCDECL1(INT32, nativeGetTwoDigitYearMax, INT32 nValue);

    //
    //  Native helper function for methods in TimeZone
    //
    static FCDECL0(LONG, nativeGetTimeZoneMinuteOffset);
    static FCDECL0(Object*, nativeGetStandardName);
    static FCDECL0(Object*, nativeGetDaylightName);
    static FCDECL0(Object*, nativeGetDaylightChanges);
    
    //
    //  Native helper function for methods in TextInfo
    //
    static FCDECL0(INT32, nativeGetNumEncodingItems);
    static FCDECL0(EncodingDataItem *, nativeGetEncodingTableDataPointer);
    static FCDECL0(CodePageDataItem *, nativeGetCodePageTableDataPointer);

    //
    //  Native helper function for methods in CharacterInfo
    //
    static FCDECL0(void, AllocateCharTypeTable);

    //
    //  Native helper functions for methods in GlobalizationAssembly.
    //
    static FCDECL1(LPVOID, nativeCreateGlobalizationAssembly, AssemblyBaseObject* pAssembly);

    //
    //  Native helper functions for methods in CompareInfo.
    //
    static FCDECL2(LPVOID, InitializeNativeCompareInfo, INT_PTR pNativeGlobalizationAssembly, INT32 sortID);

    //
    //  Native helper function for methods in TextInfo
    //
    static FCDECL4(INT32, nativeChangeCaseChar, INT32, INT_PTR, WCHAR, BYTE);
    static FCDECL2(INT32, nativeGetTitleCaseChar, INT_PTR , WCHAR);

    
    static FCDECL4(Object*, nativeChangeCaseString, INT32 nLCID, INT_PTR pNativeTextInfo, StringObject* pString, BYTE bIsToUpper);
    static FCDECL0(LPVOID, AllocateDefaultCasingTable);
    static FCDECL1(LPVOID, AllocateCasingTable, INT32 lcid);



    static CasingTable* m_pCasingTable;

private:

    //
    //  Internal helper functions.
    //
    static LPVOID internalEnumSystemLocales(DWORD dwFlags);
    static PTRARRAYREF GetMultiStringValues(LPWSTR pInfoStr);
    static INT32  CompareFast(STRINGREF strA, STRINGREF strB, BOOL *pbDifferInCaseOnly);
    static INT32 CompareOrdinal(WCHAR* strAChars, int Length1, WCHAR* strBChars, int Length2 );
    static INT32 __stdcall  DoCompareChars(WCHAR charA, WCHAR charB, BOOL *bDifferInCaseOnly);
    static inline INT32  DoComparisonLookup(wchar_t charA, wchar_t charB);
    static void   ConvertStringCaseFast(WCHAR *inBuff, WCHAR* outBuff, INT32 length, DWORD dwOptions);
    static INT32  FastIndexOfString(WCHAR *sourceString, INT32 startIndex, INT32 endIndex, WCHAR *pattern, INT32 patternLength);
    static INT32  FastIndexOfStringInsensitive(WCHAR *sourceString, INT32 startIndex, INT32 endIndex, WCHAR *pattern, INT32 patternLength);
    static INT32  FastLastIndexOfString(WCHAR *sourceString, INT32 startIndex, INT32 endIndex, WCHAR *pattern, INT32 patternLength);
    static INT32  FastLastIndexOfStringInsensitive(WCHAR *sourceString, INT32 startIndex, INT32 endIndex, WCHAR *pattern, INT32 patternLength);

    //
    //  Definitions.
    //
    
    #define CULTUREINFO_OPTIONS_SIZE 32
    
    const static WCHAR ToUpperMapping[];
    const static WCHAR ToLowerMapping[];
    const static INT8 ComparisonTable[0x80][0x80];



private:  
    //
    // Internal encoding data tables.
    //
    const static int m_nEncodingDataTableItems;
    const static EncodingDataItem EncodingDataTable[];
    
    const static int m_nCodePageTableItems;
    const static CodePageDataItem CodePageDataTable[];

};


class NativeTextInfo; //Defined in clr\src\ClassLibNative\NLS;

class InternalCasingHelper {
    private:
    static NativeTextInfo* pNativeTextInfo;
    static void InitTable();

    public:
    //
    // Native helper functions to do correct casing operations in 
    // runtime native code.
    //

    // Convert szIn to lower case in the Invariant locale.
    static INT32 InvariantToLower(LPUTF8 szOut, int cMaxBytes, LPCUTF8 szIn);
};
#endif  // _COMNLSINFO_H
