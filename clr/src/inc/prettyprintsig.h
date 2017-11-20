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
// This code supports formatting a method and it's signature in a friendly
// and consistent format.
//
//*****************************************************************************
#ifndef __PrettyPrintSig_h__
#define __PrettyPrintSig_h__

#include <cor.h>

class CQuickBytes;


LPCWSTR PrettyPrintSig(
	PCCOR_SIGNATURE typePtr,			// type to convert, 	
	unsigned	typeLen,				// length of type
	LPCWSTR 	name,					// can be L"", the name of the method for this sig	
	CQuickBytes *out,					// where to put the pretty printed string	
	IMetaDataImport *pIMDI);			// Import api to use.

struct IMDInternalImport;
HRESULT PrettyPrintSigInternal(         // S_OK or error.
	PCCOR_SIGNATURE typePtr,			// type to convert, 	
	unsigned	typeLen,				// length of type
	LPCSTR 	name,					    // can be "", the name of the method for this sig	
	CQuickBytes *out,					// where to put the pretty printed string	
	IMDInternalImport *pIMDI);			// Import api to use.


#endif // __PrettyPrintSig_h__
