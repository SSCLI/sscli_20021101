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
// ===========================================================================
// File: metahelp.h
//
// Helper routines for importing/emitting CLR metadata.
// ===========================================================================

class METADATAHELPER
{
public:
    static bool GetFullMetadataName(PSYM sym, 
                                WCHAR * typeName,  int cchTypeName,
                                WCHAR chNestedClassSep = NESTED_CLASS_SEP,
                                WCHAR chDotReplacement = L'.');
    static DWORD GetAggregateFlags(AGGSYM * sym);
    static void GetExplicitImplName(SYM * sym, WCHAR * buffer, int cch);
};

