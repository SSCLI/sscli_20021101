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
#ifndef _COMFLOATCLASS_H
#define _COMFLOATCLASS_H

#include <object.h>
#include <fcall.h>


#define POSITIVE_INFINITY_POS 0
#define NEGATIVE_INFINITY_POS 1
#define NAN_POS 2

class COMFloat {

public:
    FCDECL1(static INT32, IsNAN, float d);
    FCDECL1(static INT32, IsInfinity, float d);
    FCDECL1(static INT32, IsNegativeInfinity, float d);
    FCDECL1(static INT32, IsPositiveInfinity, float d);
};

class COMDouble {
public:
    FCDECL1(static INT32, IsNAN, double d);
    FCDECL1(static INT32, IsInfinity, double d);
    FCDECL1(static INT32, IsNegativeInfinity, double d);
    FCDECL1(static INT32, IsPositiveInfinity, double d);

    FCDECL1(static R8, Floor, double d);
    FCDECL1(static R8, Sqrt, double d);
    FCDECL1(static R8, Log, double d);
    FCDECL1(static R8, Log10, double d);
    FCDECL1(static R8, Exp, double d);
    FCDECL2_VV(static R8, Pow, double x, double y);
    FCDECL1(static R8, Acos, double d);
    FCDECL1(static R8, Asin, double d);
    FCDECL1(static R8, Atan, double d);
    FCDECL2_VV(static R8, Atan2, double x, double y);
    FCDECL1(static R8, Cos, double d);
    FCDECL1(static R8, Sin, double d);
    FCDECL1(static R8, Tan, double d);
    FCDECL1(static R8, Cosh, double d);
    FCDECL1(static R8, Sinh, double d);
    FCDECL1(static R8, Tanh, double d);
    FCDECL1(static R8, Round, double d);
    FCDECL1(static R8, Ceil, double d);
    FCDECL1(static R4, AbsFlt, float f);
    FCDECL1(static R8, AbsDbl, double d);
    FCDECL2_VI(static R8, RoundDigits, double dblIn,int cDecimals);
    FCDECL2_VV(static R8, IEEERemainder, double x, double y);

//private:

    FCDECL2_VV(static R8, PowHelper, double x, double y);

};
    

#endif // _COMFLOATCLASS_H

