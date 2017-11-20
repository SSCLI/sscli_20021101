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
#include "common.h"
#include "object.h"
#include "excep.h"
#include "frames.h"
#include "vars.hpp"
#include "comdecimal.h"
#include "comstring.h"

FCIMPL2 (void, COMDecimal::InitSingle, DECIMAL *_this, R4 value)
{
    if (VarDecFromR4(value, _this) < 0)
        FCThrowResVoid(kOverflowException, L"Overflow_Decimal");
    _this->wReserved = 0;
}
FCIMPLEND

FCIMPL2 (void, COMDecimal::InitDouble, DECIMAL *_this, R8 value)
{
    if (VarDecFromR8(value, _this) < 0)
        FCThrowResVoid(kOverflowException, L"Overflow_Decimal");
    _this->wReserved = 0;
}
FCIMPLEND


FCIMPL2_VV_RET_VC(DECIMAL, result, COMDecimal::Add, DECIMAL d1, DECIMAL d2)
{
    if (VarDecAdd(&d1, &d2, result) < 0)
        FCThrowResRetVC(kOverflowException, L"Overflow_Decimal");
    result->wReserved = 0;
}
FCIMPLEND_RET_VC

FCIMPL2_VV(INT32, COMDecimal::Compare, DECIMAL d1, DECIMAL d2)
{
    return VarDecCmp(&d1, &d2) - 1;
}
FCIMPLEND

FCIMPL2_VV_RET_VC(DECIMAL, result, COMDecimal::Divide, DECIMAL d1, DECIMAL d2)
{
    HRESULT hr = VarDecDiv(&d1, &d2, result);
    if (hr < 0) {
        if (hr == DISP_E_DIVBYZERO) 
            FCThrowRetVC(kDivideByZeroException);
        FCThrowResRetVC(kOverflowException, L"Overflow_Decimal");
    }
    result->wReserved = 0;
}
FCIMPLEND_RET_VC

FCIMPL1_RET_VC(DECIMAL, result, COMDecimal::Floor, DECIMAL d)
{
#ifdef _DEBUG
	HRESULT hr =
#endif
    VarDecInt(&d, result);

    // VarDecInt can't overflow, as of source for OleAut32 build 4265.
    // It only returns NOERROR
    _ASSERTE(hr==NOERROR);
}
FCIMPLEND_RET_VC

FCIMPL1(INT32, COMDecimal::GetHashCode, DECIMAL *d)
{
    double dbl;
    VarR8FromDec(d, &dbl);
    return ((int *)&dbl)[0] ^ ((int *)&dbl)[1];
}
FCIMPLEND

FCIMPL2_VV_RET_VC(DECIMAL, result, COMDecimal::Remainder, DECIMAL d1, DECIMAL d2)
{
    HRESULT hr = S_FALSE;
    // OleAut doesn't provide a VarDecMod.
    // Formula:  d1 - (RoundTowardsZero(d1 / d2) * d2)
	DECIMAL tmp;
	// This piece of code is to work around the fact that Dividing a decimal with 28 digits number by decimal which causes
	// causes the result to be 28 digits, can cause to be incorrectly rounded up.
	// eg. Decimal.MaxValue / 2 * Decimal.MaxValue will overflow since the division by 2 was rounded instead of being truncked.
	// In the operation x % y the sign of y does not matter. Result will have the sign of x.
	if (DECIMAL_SIGN(d1) == 0) {
		DECIMAL_SIGN(d2) = 0;
		if (VarDecCmp(&d1,&d2) < 1) {
			*result = d1;
            goto lExit;
		}
	} else {
		DECIMAL_SIGN(d2) = 0x80; // Set the sign bit to negative
		if (VarDecCmp(&d1,&d2) > 1) {
			*result = d1;
            goto lExit;
		}
	}

	VarDecSub(&d1, &d2, &tmp);

	d1 = tmp;
	hr = VarDecDiv(&d1, &d2, &tmp);
    if (hr < 0) {
        if (hr == DISP_E_DIVBYZERO) 
            FCThrowRetVC(kDivideByZeroException);
        FCThrowResRetVC(kOverflowException, L"Overflow_Decimal");
    }
    // VarDecFix rounds towards 0.
    hr = VarDecFix(&tmp, result);
    if (FAILED(hr)) {
        _ASSERTE(!"VarDecFix failed in Decimal::Mod");
        FCThrowResRetVC(kOverflowException, L"Overflow_Decimal");
    }

    hr = VarDecMul(&d2, result, &tmp);
    if (FAILED(hr)) {
        _ASSERTE(!"VarDecMul failed in Decimal::Mod");
        FCThrowResRetVC(kOverflowException, L"Overflow_Decimal");
    }

    hr = VarDecSub(&d1, &tmp, result);
    if (FAILED(hr)) {
        _ASSERTE(!"VarDecSub failed in Decimal::Mod");
        FCThrowResRetVC(kOverflowException, L"Overflow_Decimal");
    }
    result->wReserved = 0;

lExit: ;
}
FCIMPLEND_RET_VC

FCIMPL2_VV_RET_VC(DECIMAL, result, COMDecimal::Multiply, DECIMAL d1, DECIMAL d2)
{
    if (VarDecMul(&d1, &d2, result) < 0)
        FCThrowResRetVC(kOverflowException, L"Overflow_Decimal");
    result->wReserved = 0;
}
FCIMPLEND_RET_VC

FCIMPL2_VI_RET_VC(DECIMAL, result, COMDecimal::Round, DECIMAL d, INT32 decimals)
{
    if (decimals < 0 || decimals > 28)
        FCThrowArgumentOutOfRangeRetVC(L"decimals", L"ArgumentOutOfRange_DecimalRound");
    if (VarDecRound(&d, decimals, result) < 0)
        FCThrowResRetVC(kOverflowException, L"Overflow_Decimal");
    result->wReserved = 0;
}
FCIMPLEND_RET_VC

FCIMPL2_VV_RET_VC(DECIMAL, result, COMDecimal::Subtract, DECIMAL d1, DECIMAL d2)
{
    if (VarDecSub(&d1, &d2, result) < 0)
        FCThrowResRetVC(kOverflowException, L"Overflow_Decimal");
    result->wReserved = 0;
}
FCIMPLEND_RET_VC

FCIMPL1_RET_VC(CY, result, COMDecimal::ToCurrency, DECIMAL d)
{
    HRESULT hr;
    if ((hr = VarCyFromDec(&d, result)) < 0) {
        _ASSERTE(hr != E_INVALIDARG);
        FCThrowResRetVC(kOverflowException, L"Overflow_Currency");
    }
}
FCIMPLEND_RET_VC

FCIMPL1(double, COMDecimal::ToDouble, DECIMAL d)
{
    double result;
    VarR8FromDec(&d, &result);
    return result;
}
FCIMPLEND

FCIMPL1(float, COMDecimal::ToSingle, DECIMAL d)
{
    float result;
    VarR4FromDec(&d, &result);
    return result;
}
FCIMPLEND

FCIMPL1(Object*, COMDecimal::ToString, DECIMAL d)
{
    STRINGREF result = NULL;
    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, result);

    BSTR bstr;
    VarBstrFromDec(&d, 0, 0, &bstr);
    result = COMString::NewString(bstr, SysStringLen(bstr));
    SysFreeString(bstr);

    HELPER_METHOD_FRAME_END();

    return OBJECTREFToObject(result);
}
FCIMPLEND

FCIMPL1_RET_VC(DECIMAL, result, COMDecimal::Truncate, DECIMAL d)
{
    VarDecFix(&d, result);
}
FCIMPLEND_RET_VC
