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
// MDCommon.h
//
// Common header file for both MD and COMPLIB subdirectories
//
//*****************************************************************************
#ifndef __MDCommon_h__
#define __MDCommon_h__

// File types for the database.
enum FILETYPE
{	
	FILETYPE_UNKNOWN,					// Unknown or undefined type.
	FILETYPE_CLB,						// Native .clb file format.
	FILETYPE_CLX, 					    // An obsolete file format.
	FILETYPE_NTPE,						// Windows PE executable.
	FILETYPE_NTOBJ, 					// .obj file format (with .clb embedded).
	FILETYPE_TLB						// Typelib format.
};


#define SCHEMA_STREAM_A				"#Schema"
#define STRING_POOL_STREAM_A		"#Strings"
#define BLOB_POOL_STREAM_A			"#Blob"
#define US_BLOB_POOL_STREAM_A		"#US"
#define VARIANT_POOL_STREAM_A 		"#Variants"
#define GUID_POOL_STREAM_A			"#GUID"
#define COMPRESSED_MODEL_STREAM_A	"#~"
#define ENC_MODEL_STREAM_A			"#-"

#define SCHEMA_STREAM				L"#Schema"
#define STRING_POOL_STREAM			L"#Strings"
#define BLOB_POOL_STREAM			L"#Blob"
#define US_BLOB_POOL_STREAM			L"#US"
#define VARIANT_POOL_STREAM			L"#Variants"
#define GUID_POOL_STREAM			L"#GUID"
#define COMPRESSED_MODEL_STREAM		L"#~"
#define ENC_MODEL_STREAM			L"#-"

#endif // __MDCommon_h__
