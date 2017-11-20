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
#ifndef _COMCURRENCY_H_
#define _COMCURRENCY_H_

#include <oleauto.h>

#include <pshpack1.h>

class COMCurrency {
public:
    //struct InitSingleArgs {
    //    DECLARE_ECALL_PTR_ARG(CY*, _this);
    //    DECLARE_ECALL_R4_ARG(R4, value);
    //};

    //struct InitDoubleArgs {
    //    DECLARE_ECALL_PTR_ARG(CY*, _this);
    //    DECLARE_ECALL_R8_ARG(R8, value);
    //};

    //struct InitStringArgs {
    //    DECLARE_ECALL_PTR_ARG(CY*, _this);
    //    DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, value);
    //};

    //struct ArithOpArgs {
    //    DECLARE_ECALL_DEFAULT_ARG(CY, c2);
    //    DECLARE_ECALL_DEFAULT_ARG(CY, c1);
    //    DECLARE_ECALL_PTR_ARG(CY*, result);
    //};

    //struct FloorArgs {
    //    DECLARE_ECALL_DEFAULT_ARG(CY, c);
    //    DECLARE_ECALL_PTR_ARG(CY*, result);
    //};

    //struct RoundArgs {
    //    DECLARE_ECALL_I4_ARG(I4, decimals);
    //    DECLARE_ECALL_DEFAULT_ARG(CY, c);
    //    DECLARE_ECALL_PTR_ARG(CY*, result);
    //};

    //struct ToDecimalArgs {
    //    DECLARE_ECALL_DEFAULT_ARG(CY, c);
    //    DECLARE_ECALL_PTR_ARG(DECIMAL*, result);
    //};

    //struct ToXXXArgs {
    //    DECLARE_ECALL_DEFAULT_ARG(CY, c);
    //};

    //struct TruncateArgs {
    //    DECLARE_ECALL_DEFAULT_ARG(CY, c);
    //    DECLARE_ECALL_PTR_ARG(CY*, result);
    //};

    static FCDECL2(void, InitSingle, CY* _this, R4 value);
    static FCDECL2(void, InitDouble, CY* _this, R8 value);
    static FCDECL2(void, InitString, CY* _this, StringObject* valueUNSAFE);

    static FCDECL3(void, Add, CY* result, CY c1, CY c2);
    static FCDECL2(void, Floor, CY* result, CY c);
    static FCDECL3(void, Multiply, CY* result, CY c1, CY c2);
    static FCDECL3(void, Round, CY* result, CY c, I4 decimals);
    static FCDECL3(void, Subtract, CY* result, CY c1, CY c2);
    static FCDECL2(void, ToDecimal, DECIMAL* result, CY c);

    static FCDECL1(double, ToDouble, CY c);
    static FCDECL1(float, ToSingle, CY c);
    static FCDECL1(Object*, ToString, CY c);

    static FCDECL2(void, Truncate, CY* result, CY c);
};

#include <poppack.h>

#endif // _COMCURRENCY_H_
