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
#include <common.h>

#include "comfloat.h"
#include "comfloatclass.h"

// This table is required for the Round function which can specify the number of digits to round to
#define MAX_ROUND_DBL 15  // largest power that's exact in a double

static const double rgdblPower10[MAX_ROUND_DBL + 1] = 
    {
      1E0, 1E1, 1E2, 1E3, 1E4, 1E5, 1E6, 1E7, 1E8,
      1E9, 1E10, 1E11, 1E12, 1E13, 1E14, 1E15
    };

/*==============================================================================
**Floats (single precision) in IEEE754 format are represented as follows.
**  ----------------------------------------------------------------------
**  | Sign(1 bit) |  Exponent (8 bits) |   Significand (23 bits)         |
**  ----------------------------------------------------------------------
**
**NAN is indicated by setting the sign bit, all 8 exponent bits, and the high order
**bit of the significand.   This yields the key value of 0xFFC00000.
**
**Positive Infinity is indicated by setting all 8 exponent bits to 1 and all others
**to 0.  This yields a key value of 0x7F800000.
**
**Negative Infinity is the same as positive infinity except with the sign bit set.
==============================================================================*/



/*===============================IsInfinityFloat================================
**Args:    typedef struct {R4 flt;} _singleFloatArgs;
**Returns: True if args->flt is Infinity.  False otherwise.  We don't care at this
**         point if its positive or negative infinity. See above for the 
**         description of value used to determine Infinity.
**Exceptions: None.
==============================================================================*/
FCIMPL1(INT32, COMFloat::IsInfinity, float f)
    //C doesn't like casting a float directly to an Unsigned Int *, so cast it to
    //a void * first, and then to an unsigned int * and then dereference it.
    return  ((*((UINT32 *)((void *)&f)) == FLOAT_POSITIVE_INFINITY)||
            (*((UINT32 *)((void *)&f)) == FLOAT_NEGATIVE_INFINITY));
FCIMPLEND


/*===========================IsNegativeInfinityFloat============================
**Args:    typedef struct {R4 flt;} _singleFloatArgs;
**Returns: True if args->flt is Infinity.  False otherwise. See above for the 
**         description of value used to determine Negative Infinity.
**Exceptions: None
==============================================================================*/
FCIMPL1(INT32, COMFloat::IsNegativeInfinity, float f)
    //C doesn't like casting a float directly to an Unsigned Int *, so cast it to
    //a void * first, and then to an unsigned int * and then dereference it.
    return (*((UINT32 *)((void *)&f)) == FLOAT_NEGATIVE_INFINITY);
FCIMPLEND


/*===========================IsPositiveInfinityFloat============================
**Args:    typedef struct {R4 flt;} _singleFloatArgs;
**Returns: True if args->flt is Infinity.  False otherwise. See above for the 
**         description of value used to determine Positive Infinity.
**Exceptions: None
==============================================================================*/
FCIMPL1(INT32, COMFloat::IsPositiveInfinity, float f)
    //C doesn't like casting a float directly to an Unsigned Int *, so cast it to
    //a void * first, and then to an unsigned int * and then dereference it.
    return  (*((UINT32 *)((void *)&f)) == FLOAT_POSITIVE_INFINITY);
FCIMPLEND

//
//
// DOUBLE PRECISION OPERATIONS
//
//

/*===============================IsInfinityDouble===============================
**Args:    typedef struct {R8 dbl;} _singleDoubleArgs;
**Returns: True if args->flt is Infinity.  False otherwise.  We don't care at this
**         point if its positive or negative infinity. See above for the 
**         description of value used to determine Infinity.
**Exceptions: None.
==============================================================================*/
FCIMPL1(INT32, COMDouble::IsInfinity, double d)
    //C doesn't like casting a double directly to an UINT64 *, so cast it to
    //a void * first, and then to an UINT * and then dereference it.
    return  ((*((UINT64 *)((void *)&d)) == DOUBLE_POSITIVE_INFINITY)||
            (*((UINT64 *)((void *)&d)) == DOUBLE_NEGATIVE_INFINITY));
FCIMPLEND

/*===========================IsNegativeInfinityFloat============================
**Args:    typedef struct {R4 flt;} _singleFloatArgs;
**Returns: True if args->flt is Infinity.  False otherwise. See above for the 
**         description of value used to determine Negative Infinity.
**Exceptions: None
==============================================================================*/
FCIMPL1(INT32, COMDouble::IsNegativeInfinity, double d)
    //C doesn't like casting a double directly to an UINT64 *, so cast it to
    //a void * first, and then to an UINT64 * and then dereference it.
    return (*((UINT64 *)((void *)&d)) == DOUBLE_NEGATIVE_INFINITY);
FCIMPLEND

/*===========================IsPositiveInfinityFloat============================
**Args:    typedef struct {R4 flt;} _singleFloatArgs;
**Returns: True if args->flt is Infinity.  False otherwise. See above for the 
**         description of value used to determine Positive Infinity.
**Exceptions: None
==============================================================================*/
FCIMPL1(INT32, COMDouble::IsPositiveInfinity, double d)
    //C doesn't like casting a double directly to an UINT64 *, so cast it to
    //a void * first, and then to an UINT64 * and then dereference it.
    return  (*((UINT64 *)((void *)&d)) == DOUBLE_POSITIVE_INFINITY);
FCIMPLEND

/*====================================Floor=====================================
**
==============================================================================*/
FCIMPL1(R8, COMDouble::Floor, double d) 
    return (R8) floor(d);
FCIMPLEND


/*====================================Ceil=====================================
**
==============================================================================*/
FCIMPL1(R8, COMDouble::Ceil, double d) 
    return (R8) ceil(d);
FCIMPLEND

/*=====================================Sqrt=====================================
**
==============================================================================*/
FCIMPL1(R8, COMDouble::Sqrt, double d) 
    return (R8) sqrt(d);
FCIMPLEND

/*=====================================Log======================================
**This is the natural log
==============================================================================*/
FCIMPL1(R8, COMDouble::Log, double d) 
    return (R8) log(d);
FCIMPLEND


/*====================================Log10=====================================
**This is log-10
==============================================================================*/
FCIMPL1(R8, COMDouble::Log10, double d) 
    return (R8) log10(d);
FCIMPLEND

/*=====================================Pow======================================
**This is the power function.  Simple powers are done inline, and special
  cases are sent to the CRT via the helper.                                        
==============================================================================*/
FCIMPL2_VV(R8, COMDouble::PowHelper, double x, double y) 
{
    return (R8) pow(x, y);
}
FCIMPLEND

FCIMPL2_VV(R8, COMDouble::Pow, double x, double y)
{
    return (R8) pow(x, y);
}
FCIMPLEND


/*=====================================Exp======================================
**
==============================================================================*/
FCIMPL1(R8, COMDouble::Exp, double x) 

    // The C intrinsic below does not handle +- infinity properly
    // so we handle these specially here
    if ((*((UINT64 *)((void *)&x)) == DOUBLE_POSITIVE_INFINITY))
    {
        return x;
    }
    else if ((*((UINT64 *)((void *)&x)) == DOUBLE_NEGATIVE_INFINITY))
    {
        return(+0.0);
    }

    return((R8) exp(x));

FCIMPLEND

/*=====================================Acos=====================================
**
==============================================================================*/
FCIMPL1(R8, COMDouble::Acos, double d) 
    return (R8) acos(d);
FCIMPLEND


/*=====================================Asin=====================================
**
==============================================================================*/
FCIMPL1(R8, COMDouble::Asin, double d) 
    return (R8) asin(d);
FCIMPLEND


/*=====================================AbsFlt=====================================
**
==============================================================================*/
FCIMPL1(R4, COMDouble::AbsFlt, float f) 
    return fabsf(f);
FCIMPLEND

/*=====================================AbsDbl=====================================
**
==============================================================================*/
FCIMPL1(R8, COMDouble::AbsDbl, double d) 
    return fabs(d);
FCIMPLEND

/*=====================================Atan=====================================
**
==============================================================================*/
FCIMPL1(R8, COMDouble::Atan, double d) 
    return (R8) atan(d);
FCIMPLEND

/*=====================================Atan2=====================================
**
==============================================================================*/
FCIMPL2_VV(R8, COMDouble::Atan2, double x, double y) 

    // the intrinsic for Atan2 does not produce Nan for Atan2(+-inf,+-inf)
    if ((((*((UINT64 *)((void *)&x)) == DOUBLE_POSITIVE_INFINITY)||
         (*((UINT64 *)((void *)&x)) == DOUBLE_NEGATIVE_INFINITY))) &&
        (((*((UINT64 *)((void *)&y)) == DOUBLE_POSITIVE_INFINITY)||
         (*((UINT64 *)((void *)&y)) == DOUBLE_NEGATIVE_INFINITY))))
    {
        return(x / y);      // create a NaN
    }
    return (R8) atan2(x, y);
FCIMPLEND

/*=====================================Sin=====================================
**
==============================================================================*/
FCIMPL1(R8, COMDouble::Sin, double d) 
    return (R8) sin(d);
FCIMPLEND

/*=====================================Cos=====================================
**
==============================================================================*/
FCIMPL1(R8, COMDouble::Cos, double d) 
    return (R8) cos(d);
FCIMPLEND

/*=====================================Tan=====================================
**
==============================================================================*/
FCIMPL1(R8, COMDouble::Tan, double d) 
    return (R8) tan(d);
FCIMPLEND

/*=====================================Sinh====================================
**
==============================================================================*/
FCIMPL1(R8, COMDouble::Sinh, double d) 
    return (R8) sinh(d);
FCIMPLEND

/*=====================================Cosh====================================
**
==============================================================================*/
FCIMPL1(R8, COMDouble::Cosh, double d) 
    return (R8) cosh(d);
FCIMPLEND

/*=====================================Tanh====================================
**
==============================================================================*/
FCIMPL1(R8, COMDouble::Tanh, double d) 
    return (R8) tanh(d);
FCIMPLEND

/*=====================================IEEERemainder===========================
**
==============================================================================*/
FCIMPL2_VV(R8, COMDouble::IEEERemainder, double x, double y) 
    return (R8) fmod(x, y);
FCIMPLEND

/*====================================Round=====================================
**
==============================================================================*/
FCIMPL1(R8, COMDouble::Round, double d) 
    R8 tempVal;
    R8 flrTempVal;
    // If the number has no fractional part do nothing
    if ( d == (double)(__int64)d )
          return d;
    tempVal = (d+0.5);
    //We had a number that was equally close to 2 integers. 
    //We need to return the even one.
    flrTempVal = floor(tempVal);
    if (flrTempVal==tempVal) {
        if (0==fmod(tempVal, 2.0)) {
            return flrTempVal;
        }
        return flrTempVal-1.0;
    }
    return flrTempVal;
FCIMPLEND


// We do the bounds checking in managed code to ensure that we have between 0 and 15 in cDecimals.
FCIMPL2_VI(R8, COMDouble::RoundDigits, double dblIn, int cDecimals)
    if (fabs(dblIn) < 1E16)
    {
      dblIn *= rgdblPower10[cDecimals];

      double      dblFrac;

      dblFrac = modf(dblIn, &dblIn);
      if (fabs(dblFrac) != 0.5 || fmod(dblIn, 2) != 0)
    dblIn += (int)(dblFrac * 2);

      dblIn /= rgdblPower10[cDecimals];
    }
    return dblIn;
FCIMPLEND

