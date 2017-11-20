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
#ifndef _COMDECIMAL_H_
#define _COMDECIMAL_H_

#include <oleauto.h>

#include <pshpack1.h>

class COMDecimal {
public:
    static FCDECL2(void, InitSingle, DECIMAL *_this, R4 value);
    static FCDECL2(void, InitDouble, DECIMAL *_this, R8 value);
    static FCDECL2_VV(INT32, Compare, DECIMAL d1, DECIMAL d2);
    static FCDECL1(INT32, GetHashCode, DECIMAL *d);

    static FCDECL2_VV_RET_VC(DECIMAL, result, Add,         DECIMAL d1, DECIMAL d2);
    static FCDECL2_VV_RET_VC(DECIMAL, result, Subtract,    DECIMAL d1, DECIMAL d2);
    static FCDECL2_VV_RET_VC(DECIMAL, result, Divide,      DECIMAL d1, DECIMAL d2);
    static FCDECL2_VV_RET_VC(DECIMAL, result, Remainder,   DECIMAL d1, DECIMAL d2);
    static FCDECL2_VV_RET_VC(DECIMAL, result, Multiply,    DECIMAL d1, DECIMAL d2);
    static FCDECL2_VI_RET_VC(DECIMAL, result, Round,       DECIMAL d, INT32 decimals);
    static FCDECL1_RET_VC(CY,      result, ToCurrency,  DECIMAL d);
    static FCDECL1_RET_VC(DECIMAL, result, Truncate,    DECIMAL d);
    static FCDECL1_RET_VC(DECIMAL, result, Floor,       DECIMAL d);

    static FCDECL1(double, ToDouble, DECIMAL d);
    static FCDECL1(float, ToSingle, DECIMAL d);
    static FCDECL1(Object*, ToString, DECIMAL d);
};

#include <poppack.h>

#endif // _COMDECIMAL_H_
