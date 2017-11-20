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
#ifndef _COMNUMBER_H_
#define _COMNUMBER_H_

#include "common.h"

#include <pshpack1.h>

class NumberFormatInfo: public Object
{
public:
    // C++ data members                 // Corresponding data member in NumberFormat.cs
                                        // Also update mscorlib.h when you add/remove fields

    I4ARRAYREF cNumberGroup;		// numberGroupSize
    I4ARRAYREF cCurrencyGroup;		// currencyGroupSize
    I4ARRAYREF cPercentGroup;		// percentGroupSize
    
    STRINGREF sPositive;           	// positiveSign
    STRINGREF sNegative;           	// negativeSign
    STRINGREF sNumberDecimal;      	// numberDecimalSeparator
    STRINGREF sNumberGroup;        	// numberGroupSeparator
    STRINGREF sCurrencyGroup;      	// currencyDecimalSeparator
    STRINGREF sCurrencyDecimal;    	// currencyGroupSeparator
    STRINGREF sCurrency;            // currencySymbol
    STRINGREF sAnsiCurrency;        // ansiCurrencySymbol
    STRINGREF sNaN;                 // nanSymbol
    STRINGREF sPositiveInfinity;    // positiveInfinitySymbol
    STRINGREF sNegativeInfinity;    // negativeInfinitySymbol
    STRINGREF sPercentDecimal;		// percentDecimalSeparator
    STRINGREF sPercentGroup;		// percentGroupSeparator
    STRINGREF sPercent;				// percentSymbol
    STRINGREF sPerMille;			// perMilleSymbol
    
    INT32 iDataItem;                // Index into the CultureInfo Table.  Only used from managed code.
    INT32 cNumberDecimals;			// numberDecimalDigits
    INT32 cCurrencyDecimals;        // currencyDecimalDigits
    INT32 cPosCurrencyFormat;       // positiveCurrencyFormat
    INT32 cNegCurrencyFormat;       // negativeCurrencyFormat
    INT32 cNegativeNumberFormat;    // negativeNumberFormat
    INT32 cPositivePercentFormat;   // positivePercentFormat
    INT32 cNegativePercentFormat;   // negativePercentFormat
    INT32 cPercentDecimals;			// percentDecimalDigits

    INT8 bIsReadOnly;              // Is this NumberFormatInfo ReadOnly?
    INT8 bUseUserOverride;         // Flag to use user override. Only used from managed code.
    INT8 bValidForParseAsNumber;   // Are the separators set unambiguously for parsing as number?
    INT8 bValidForParseAsCurrency; // Are the separators set unambiguously for parsing as currency?
    
};

typedef NumberFormatInfo * NUMFMTREF;

#define PARSE_LEADINGWHITE  0x0001
#define PARSE_TRAILINGWHITE 0x0002
#define PARSE_LEADINGSIGN   0x0004
#define PARSE_TRAILINGSIGN  0x0008
#define PARSE_PARENS        0x0010
#define PARSE_DECIMAL       0x0020
#define PARSE_THOUSANDS     0x0040
#define PARSE_SCIENTIFIC    0x0080
#define PARSE_CURRENCY      0x0100
#define PARSE_HEX	    0x0200
#define PARSE_PERCENT       0x0400

class COMNumber
{
public:
    static FCDECL3_VII(Object*, FormatDecimal, DECIMAL value, StringObject* formatUNSAFE, NumberFormatInfo* numfmtUNSAFE);
    static FCDECL3_VII(Object*, FormatDouble,  R8      value, StringObject* formatUNSAFE, NumberFormatInfo* numfmtUNSAFE);
    static FCDECL3_VII(Object*, FormatSingle,  R4      value, StringObject* formatUNSAFE, NumberFormatInfo* numfmtUNSAFE);
    static FCDECL3(Object*, FormatInt32,   I4      value, StringObject* formatUNSAFE, NumberFormatInfo* numfmtUNSAFE);
    static FCDECL3(Object*, FormatUInt32,  U4      value, StringObject* formatUNSAFE, NumberFormatInfo* numfmtUNSAFE);
    static FCDECL3_VII(Object*, FormatInt64,   I8      value, StringObject* formatUNSAFE, NumberFormatInfo* numfmtUNSAFE);
    static FCDECL3_VII(Object*, FormatUInt64,  U8      value, StringObject* formatUNSAFE, NumberFormatInfo* numfmtUNSAFE);

    static FCDECL3(int,              ParseInt32,     StringObject* valueUNSAFE, I4 options, NumberFormatInfo* numfmtUNSAFE);
    static FCDECL3(unsigned int,     ParseUInt32,    StringObject* valueUNSAFE, I4 options, NumberFormatInfo* numfmtUNSAFE);
    static FCDECL3(__int64,          ParseInt64,     StringObject* valueUNSAFE, I4 options, NumberFormatInfo* numfmtUNSAFE);
    static FCDECL3(unsigned __int64, ParseUInt64,    StringObject* valueUNSAFE, I4 options, NumberFormatInfo* numfmtUNSAFE);
    static FCDECL3(float,            ParseSingle,    StringObject* valueUNSAFE, I4 options, NumberFormatInfo* numfmtUNSAFE);
    static FCDECL3(double,           ParseDouble,    StringObject* valueUNSAFE, I4 options, NumberFormatInfo* numfmtUNSAFE);
    static FCDECL4(bool,             TryParseDouble, StringObject* valueUNSAFE, I4 options, NumberFormatInfo* numfmtUNSAFE, double* result);
    static FCDECL3_RET_VC(DECIMAL, result,  ParseDecimal,   StringObject* valueUNSAFE, I4 options, NumberFormatInfo* numfmtUNSAFE);
};

#include <poppack.h>

#endif // _COMNUMBER_H_
