/*============================================================================
**
** Source:  test1.c
**
** Purpose: Checks that _finite correctly classifies all types
**          of floating point numbers (NaN, -Infinity, Infinity,
**          finite nonzero, unnormalized, 0, and -0)
**
** 
**  Copyright (c) 2002 Microsoft Corporation.  All rights reserved.
** 
**  The use and distribution terms for this software are contained in the file
**  named license.txt, which can be found in the root of this distribution.
**  By using this software in any fashion, you are agreeing to be bound by the
**  terms of this license.
** 
**  You must not remove this notice, or any other, from this software.
** 
**
**==========================================================================*/


#include <palsuite.h>

/*
The IEEE double precision floating point standard looks like this:

  S EEEEEEEEEEE FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF
  0 1        11 12                                                63

S is the sign bit.  The E bits are the exponent, and the 52 F bits are
the fraction.  These represent a value, V.

If E=2047 and F is nonzero, then V=NaN ("Not a number")
If E=2047 and F is zero and S is 1, then V=-Infinity
If E=2047 and F is zero and S is 0, then V=Infinity
If 0<E<2047 then V=(-1)^S * 2^(E-1023) * (1.F) where "1.F" is the binary
   number created by prefixing F with a leading 1 and a binary point.
If E=0 and F is nonzero, then V=(-1)^S * 2^(-1022) * (0.F) These are
   "unnormalized" values.
If E=0 and F is zero and S is 1, then V=-0
If E=0 and F is zero and S is 0, then V=0

*/

int __cdecl main(int argc, char **argv)
{
#ifdef BIGENDIAN
    /*non-finite numbers*/
    unsigned long lnan[2] = {0x7fffffff, 0xffffffff};
    unsigned long lnan2[2] = {0xffffffff, 0xffffffff};
    unsigned long lneginf[2] = {0xfff00000, 0x00000000};
    unsigned long linf[2] = {0x7ff00000, 0x00000000};

    /*finite numbers*/
    unsigned long lUnnormalized[2] = {0x000fffff, 0xffffffff};
    unsigned long lNegUnnormalized[2] = {0x800fffff, 0xffffffff};
    unsigned long lNegZero[2]= {0x80000000, 0x00000000};
#else   // BIGENDIAN
    /*non-finite numbers*/
    unsigned long lnan[2] = {0xffffffff, 0x7fffffff};
    unsigned long lnan2[2] = {0xffffffff, 0xffffffff};
    unsigned long lneginf[2] = {0x00000000, 0xfff00000};
    unsigned long linf[2] = {0x00000000, 0x7ff00000};

    /*finite numbers*/
    unsigned long lUnnormalized[2] = {0xffffffff, 0x000fffff};
    unsigned long lNegUnnormalized[2] = {0xffffffff, 0x800fffff};
    unsigned long lNegZero[2]= {0x00000000, 0x80000000};
#endif  // BIGENDIAN

    double nan = *(double *)lnan;
    double nan2 = *(double *)lnan2;
    double neginf = *(double *)lneginf;
    double inf = *(double *)linf;
    double unnormalized = *(double *)lUnnormalized;
    double negUnnormalized = *(double *)lNegUnnormalized;
    double negZero = *(double *)lNegZero;
    double pos = 123.456;
    double neg = -123.456;

    /*
     * Initialize the PAL and return FAIL if this fails
     */
    if (0 != (PAL_Initialize(argc, argv)))
    {
        return FAIL;
    }

    /*non-finite numbers*/
    if (_finite(nan) || _finite(nan2))
    {
        Fail ("_finite() found NAN to be finite.\n");
    }
    if (_finite(neginf))
    {
        Fail ("_finite() found negative infinity to be finite.\n");
    }
    if (_finite(inf))
    {
        Fail ("_finite() found infinity to be finite.\n");
    }


    /*finite numbers*/
    if (!_finite(unnormalized))
    {
        Fail ("_finite() found an unnormalized value to be infinite.\n");
    }
    if (!_finite(negUnnormalized))
    {
        Fail ("_finite() found a negative unnormalized value to be infinite.\n");
    }
    if (!_finite((double)0))
    {
        Fail ("_finite found zero to be infinite.\n");
    }
    if (!_finite(negZero))
    {
        Fail ("_finite() found negative zero to be infinite.\n");
    }
    if (!_finite(pos))
    {
        Fail ("_finite() found %f to be infinite.\n", pos);
    }
    if (!_finite(neg))
    {
        Fail ("_finite() found %f to be infinite.\n", neg);
    }
    PAL_Terminate();
    return PASS;
}









