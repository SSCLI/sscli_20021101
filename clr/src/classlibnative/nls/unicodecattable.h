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
#ifndef __UNICODECAT_TABLE_H
#define __UNICODECAT_TABLE_H

////////////////////////////////////////////////////////////////////////////
//
//  Class:    CharacterInfoTable
//  Purpose:  This is the class to map a view of the Unicode category table and 
//            Unicode numeric value table.  It also provides
//            methods to access the Unicode category information.
//
//  Date: 	  August 31, 1999
//
////////////////////////////////////////////////////////////////////////////

typedef struct tagLevel2Offset {
	WORD offset[16];
} Level2Offset, *PLEVEL2OFFSET;

typedef struct {
	WORD categoryTableOffset;	// Offset to the beginning of category table
	WORD numericTableOffset;	// Offset to the beginning of numeric table.
	WORD numericFloatTableOffset;	// Offset to the beginning of numeric float table. This is the result data that will be returned.
} UNICODE_CATEGORY_HEADER, *PUNICODE_CATEGORY_HEADER;

#define LEVEL1_TABLE_SIZE 256

class CharacterInfoTable : public NLSTable {
    public:
    	static CharacterInfoTable* CreateInstance();
    	static CharacterInfoTable* GetInstance();

    	
	    CharacterInfoTable();
	    ~CharacterInfoTable();


    	BYTE GetUnicodeCategory(WCHAR wch);
		LPBYTE GetCategoryDataTable();
		LPWORD GetCategoryLevel2OffsetTable();

		LPBYTE GetNumericDataTable();
		LPWORD GetNumericLevel2OffsetTable();
		double* GetNumericFloatData();
	private:
		void InitDataMembers(LPBYTE pDataTable);
	
	    static CharacterInfoTable* m_pDefaultInstance;
	    

		PUNICODE_CATEGORY_HEADER m_pHeader;
		LPBYTE m_pByteData;
		LPBYTE m_pLevel1ByteIndex;
		PLEVEL2OFFSET m_pLevel2WordOffset;

		LPBYTE m_pNumericLevel1ByteIndex;
		LPWORD m_pNumericLevel2WordOffset;
		double* m_pNumericFloatData;

	    static const CHAR m_lpFileName[];
	    static const WCHAR m_lpMappingName[];

    	HANDLE m_pMappingHandle;
};
#endif  // __UNICODECAT_TABLE_H
