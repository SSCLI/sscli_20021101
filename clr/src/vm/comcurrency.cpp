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
#include "comcurrency.h"
#include "comstring.h"

FCIMPL2(void, COMCurrency::InitSingle, CY* _this, R4 value)
{
    HELPER_METHOD_FRAME_BEGIN_0();
    //-[autocvtpro]-------------------------------------------------------

    THROWSCOMPLUSEXCEPTION();

    HRESULT hr = VarCyFromR4(value, _this);
	if (FAILED(hr)) {
		if (hr==DISP_E_OVERFLOW)
			COMPlusThrow(kOverflowException, L"Overflow_Currency");
		_ASSERTE(hr==NOERROR);
		COMPlusThrowHR(hr);
	}

    //-[autocvtepi]-------------------------------------------------------
    HELPER_METHOD_FRAME_END();
}
FCIMPLEND

FCIMPL2(void, COMCurrency::InitDouble, CY* _this, R8 value)
{
    HELPER_METHOD_FRAME_BEGIN_0();
    //-[autocvtpro]-------------------------------------------------------

    THROWSCOMPLUSEXCEPTION();

    HRESULT hr = VarCyFromR8(value, _this);
	if (FAILED(hr)) {
		if (hr==DISP_E_OVERFLOW)
			COMPlusThrow(kOverflowException, L"Overflow_Currency");
		_ASSERTE(hr==NOERROR);
		COMPlusThrowHR(hr);
	}

    //-[autocvtepi]-------------------------------------------------------
    HELPER_METHOD_FRAME_END();
}
FCIMPLEND

FCIMPL2(void, COMCurrency::InitString, CY* _this, StringObject* valueUNSAFE)
{
    STRINGREF value = (STRINGREF) valueUNSAFE;
    HELPER_METHOD_FRAME_BEGIN_1(value);
    //-[autocvtpro]-------------------------------------------------------

    THROWSCOMPLUSEXCEPTION();

    VARIANT var;
    NUMPARSE numprs;
    BYTE digits[30];
    numprs.cDig = 30;
    numprs.dwInFlags = NUMPRS_LEADING_WHITE | NUMPRS_TRAILING_WHITE |
        NUMPRS_LEADING_MINUS | NUMPRS_DECIMAL;
	HRESULT hr = VarParseNumFromStr(value->GetBuffer(), 0x0409,
		LOCALE_NOUSEROVERRIDE, &numprs, digits);
    if (SUCCEEDED(hr)) {
        if (value->GetBuffer()[numprs.cchUsed] == 0) {
            hr = VarNumFromParseNum(&numprs, digits, VTBIT_CY, &var);
            if (SUCCEEDED(hr)) {
                *_this = V_CY(&var);
                goto lExit;
            }
        }
    }
	if (hr==DISP_E_TYPEMISMATCH)
		COMPlusThrow(kFormatException, L"Format_CurrencyBad");
	else if (hr==DISP_E_OVERFLOW)
		COMPlusThrow(kOverflowException, L"Overflow_Currency");
	else {
		_ASSERTE(hr==NOERROR);
		COMPlusThrowHR(hr);
	}

lExit: ;
    //-[autocvtepi]-------------------------------------------------------
    HELPER_METHOD_FRAME_END();
}
FCIMPLEND

FCIMPL3(void, COMCurrency::Add, CY* result, CY c1, CY c2)
{
    HELPER_METHOD_FRAME_BEGIN_0();
    //-[autocvtpro]-------------------------------------------------------

    THROWSCOMPLUSEXCEPTION();

    HRESULT hr = VarCyAdd(c1, c2, result);
	if (FAILED(hr)) {
		if (hr==DISP_E_OVERFLOW)
			COMPlusThrow(kOverflowException, L"Overflow_Currency");
		_ASSERTE(hr==NOERROR);
		COMPlusThrowHR(hr);
	}

    //-[autocvtepi]-------------------------------------------------------
    HELPER_METHOD_FRAME_END();
}
FCIMPLEND

FCIMPL2(void, COMCurrency::Floor, CY* result, CY c)
{
    HELPER_METHOD_FRAME_BEGIN_0();
    //-[autocvtpro]-------------------------------------------------------

    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(result);
	HRESULT hr = VarCyInt(c, result);
	if (FAILED(hr)) {
		if (hr==DISP_E_OVERFLOW)
			COMPlusThrow(kOverflowException, L"Overflow_Currency");
		_ASSERTE(hr==NOERROR);
		COMPlusThrowHR(hr);
	}

    //-[autocvtepi]-------------------------------------------------------
    HELPER_METHOD_FRAME_END();
}
FCIMPLEND

FCIMPL3(void, COMCurrency::Multiply, CY* result, CY c1, CY c2)
{
    HELPER_METHOD_FRAME_BEGIN_0();
    //-[autocvtpro]-------------------------------------------------------

    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(result);
    HRESULT hr = VarCyMul(c1, c2, result);
	if (FAILED(hr)) {
		if (hr==DISP_E_OVERFLOW)
			COMPlusThrow(kOverflowException, L"Overflow_Currency");
		_ASSERTE(hr==S_OK);  // Didn't expect to get here.  Update code for this HR.
		COMPlusThrowHR(hr);
    }

    //-[autocvtepi]-------------------------------------------------------
    HELPER_METHOD_FRAME_END();
}
FCIMPLEND

FCIMPL3(void, COMCurrency::Round, CY* result, CY c, I4 decimals)
{
    HELPER_METHOD_FRAME_BEGIN_0();
    //-[autocvtpro]-------------------------------------------------------

    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(result);
    if (result == NULL)
        COMPlusThrow(kArgumentNullException, L"ArgumentNull_Generic");
    if (decimals < 0 || decimals > 4)
        COMPlusThrowArgumentOutOfRange(L"digits", L"ArgumentOutOfRange_CurrencyRound");

    HRESULT hr = VarCyRound(c, decimals, result);
    if (FAILED(hr)) {
        if (hr==E_INVALIDARG)
            COMPlusThrow(kArgumentException, L"Argument_InvalidValue");
        if (hr==DISP_E_OVERFLOW)
            COMPlusThrow(kOverflowException, L"Overflow_Currency");
        _ASSERTE(hr==S_OK);  // Didn't expect to get here.  Update code for this HR.
        COMPlusThrowHR(hr);
    }

    //-[autocvtepi]-------------------------------------------------------
    HELPER_METHOD_FRAME_END();
}
FCIMPLEND

FCIMPL3(void, COMCurrency::Subtract, CY* result, CY c1, CY c2)
{
    HELPER_METHOD_FRAME_BEGIN_0();
    //-[autocvtpro]-------------------------------------------------------

    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(result);
    HRESULT hr = VarCySub(c1, c2, result);
	if (FAILED(hr)) {
		if (hr==DISP_E_OVERFLOW)
			COMPlusThrow(kOverflowException, L"Overflow_Currency");
		_ASSERTE(hr==S_OK);  // Didn't expect to get here.  Update code for this HR.
		COMPlusThrowHR(hr);
	}

    //-[autocvtepi]-------------------------------------------------------
    HELPER_METHOD_FRAME_END();
}
FCIMPLEND

FCIMPL2(void, COMCurrency::ToDecimal, DECIMAL* result, CY c)
{
    HELPER_METHOD_FRAME_BEGIN_0();
    //-[autocvtpro]-------------------------------------------------------

	THROWSCOMPLUSEXCEPTION();

    _ASSERTE(result);
    HRESULT hr = VarDecFromCy(c, result);
	if (FAILED(hr)) {
		_ASSERTE(hr==S_OK);  // Didn't expect to get here.  Update code for this HR.
		COMPlusThrowHR(hr);
	}
    result->wReserved = 0;

    //-[autocvtepi]-------------------------------------------------------
    HELPER_METHOD_FRAME_END();
}
FCIMPLEND

FCIMPL1(double, COMCurrency::ToDouble, CY c)
{
    double retVal;
    HELPER_METHOD_FRAME_BEGIN_RET_0();
    //-[autocvtpro]-------------------------------------------------------

	THROWSCOMPLUSEXCEPTION();

    HRESULT hr = VarR8FromCy(c, &retVal);
	if (FAILED(hr)) {
		_ASSERTE(hr==S_OK);  // Didn't expect to get here.  Update code for this HR.
		COMPlusThrowHR(hr);
	}

    //-[autocvtepi]-------------------------------------------------------
    HELPER_METHOD_FRAME_END();
    return retVal;
}
FCIMPLEND

FCIMPL1(float, COMCurrency::ToSingle, CY c)
{
    float retVal;
    HELPER_METHOD_FRAME_BEGIN_RET_0();
    //-[autocvtpro]-------------------------------------------------------

	THROWSCOMPLUSEXCEPTION();

    HRESULT hr = VarR4FromCy(c, &retVal);
	if (FAILED(hr)) {
		_ASSERTE(hr==S_OK);  // Didn't expect to get here.  Update code for this HR.
		COMPlusThrowHR(hr);
	}

    //-[autocvtepi]-------------------------------------------------------
    HELPER_METHOD_FRAME_END();
    return retVal;
}
FCIMPLEND

FCIMPL1(Object*, COMCurrency::ToString, CY c)
{
    STRINGREF refRetVal = NULL;
    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, refRetVal);
    //-[autocvtpro]-------------------------------------------------------

	THROWSCOMPLUSEXCEPTION();

    BSTR bstr;
    HRESULT hr = VarBstrFromCy(c, 0, 0, &bstr);
	if (FAILED(hr)) {
		if (hr==E_OUTOFMEMORY)
			COMPlusThrowOM();
		_ASSERTE(hr==S_OK);  // Didn't expect to get here.  Update code for this HR.
		COMPlusThrowHR(hr);
	}
    refRetVal = COMString::NewString(bstr, SysStringLen(bstr));
    SysFreeString(bstr);

    //-[autocvtepi]-------------------------------------------------------
    HELPER_METHOD_FRAME_END();
    return OBJECTREFToObject(refRetVal);
}
FCIMPLEND

FCIMPL2(void, COMCurrency::Truncate, CY* result, CY c)
{
    HELPER_METHOD_FRAME_BEGIN_0();
    //-[autocvtpro]-------------------------------------------------------

	THROWSCOMPLUSEXCEPTION();

    _ASSERTE(result);
    VarCyFix(c, result);
	// VarCyFix can't return anything other than NOERROR
	// currently in OleAut.

    //-[autocvtepi]-------------------------------------------------------
    HELPER_METHOD_FRAME_END();
}
FCIMPLEND
