/*++


 Copyright (c) 2002 Microsoft Corporation.  All rights reserved.

 The use and distribution terms for this software are contained in the file
 named license.txt, which can be found in the root of this distribution.
 By using this software in any fashion, you are agreeing to be bound by the
 terms of this license.

 You must not remove this notice, or any other, from this software.


Module Name:

    finite.c

Abstract:

    Implementation of _finite function (Windows specific runtime function).

--*/

#include "pal/palinternal.h"
#include "pal/dbgmsg.h"
#include <math.h>
#if HAVE_IEEEFP_H
#include <ieeefp.h>
#endif  // HAVE_IEEEFP_H
#include <errno.h>

SET_DEFAULT_DEBUG_CHANNEL(CRT);


/*++
Function:
  _finite

Determines whether given double-precision floating point value is finite.

Return Value

_finite returns a nonzero value (TRUE) if its argument x is not
infinite, that is, if -INF < x < +INF. It returns 0 (FALSE) if the
argument is infinite or a NaN.

Parameter

x  Double-precision floating-point value

--*/
int
__cdecl
_finite(
        double x)
{
    int ret;
    ENTRY("_finite (x=%f)\n", x);
    ret = finite(x);
    LOGEXIT("_finite returns int %d\n", ret);
    return ret;
}


/*++
Function:
  _isnan

See MSDN doc
--*/
int
__cdecl
_isnan(
        double x)
{
    int ret;

    ENTRY("_isnan (x=%f)\n", x);
    ret = isnan(x);
    LOGEXIT("_isnan returns int %d\n", ret);
    return ret;
}

/*++
Function:
    acos

See MSDN.
--*/
PALIMPORT double __cdecl PAL_acos(double x)
{
    double ret;

    ENTRY("acos (x=%f)\n", x);
#if !HAVE_COMPATIBLE_ACOS
    errno = 0;
#endif  // HAVE_COMPATIBLE_ACOS
    ret = acos(x);
#if !HAVE_COMPATIBLE_ACOS
    if (errno == EDOM)
    {
        if (finite(x))
        {
            ret = 0.0 / 0.0;  // NaN
        }
    }
#endif  // HAVE_COMPATIBLE_ACOS
    LOGEXIT("acos returns double %f\n", ret);
    return ret;
}

/*++
Function:
    asin

See MSDN.
--*/
PALIMPORT double __cdecl PAL_asin(double x)
{
    double ret;

    ENTRY("asin (x=%f)\n", x);
#if !HAVE_COMPATIBLE_ASIN
    errno = 0;
#endif  // HAVE_COMPATIBLE_ASIN
    ret = asin(x);
#if !HAVE_COMPATIBLE_ASIN
    if (errno == EDOM)
    {
        if (finite(x))
        {
            ret = 0.0 / 0.0;  // NaN
        }
    }
#endif  // HAVE_COMPATIBLE_ASIN
    LOGEXIT("asin returns double %f\n", ret);
    return ret;
}

/*++
Function:
    atan2

See MSDN.
--*/
PALIMPORT double __cdecl PAL_atan2(double y, double x)
{
    double ret;
    
    ENTRY("atan2 (y=%f, x=%f)\n", y, x);
#if !HAVE_COMPATIBLE_ATAN2
    errno = 0;
#endif  // !HAVE_COMPATIBLE_ATAN2
    ret = atan2(y, x);
#if !HAVE_COMPATIBLE_ATAN2
    if (errno == EDOM)
    {
#if HAVE_COPYSIGN
        if (x == 0 && copysign(1.0, x) < 0)
        {
            if (y == 0)
            {
                double pi = atan2(0, -1.0);
                if (copysign(1.0, y) > 0)
                {
                    ret = pi;
                }
                else
                {
                    ret = -pi;
                }
            }
        }
#else   // HAVE_COPYSIGN
#error  Missing copysign or equivalent on this platform!
#endif  // HAVE_COPYSIGN
    }
#endif  // !HAVE_COMPATIBLE_ATAN2
    LOGEXIT("atan2 returns double %f\n", ret);
    return ret;
}

/*++
Function:
    log

See MSDN.
--*/
PALIMPORT double __cdecl PAL_log(double x)
{
    double ret;

    ENTRY("log (x=%f)\n", x);
#if !HAVE_COMPATIBLE_LOG
    errno = 0;
#endif  // !HAVE_COMPATIBLE_LOG
    ret = log(x);
#if !HAVE_COMPATIBLE_LOG
    if (errno == EDOM)
    {
        if (x < 0)
        {
            ret = 0.0 / 0.0;    // NaN
        }
    }
#endif  // !HAVE_COMPATIBLE_LOG
    LOGEXIT("log returns double %f\n", ret);
    return ret;
}

/*++
Function:
    log10

See MSDN.
--*/
PALIMPORT double __cdecl PAL_log10(double x)
{
    double ret;

    ENTRY("log10 (x=%f)\n", x);
#if !HAVE_COMPATIBLE_LOG10
    errno = 0;
#endif  // !HAVE_COMPATIBLE_LOG10
    ret = log10(x);
#if !HAVE_COMPATIBLE_LOG10
    if (errno == EDOM)
    {
        if (x < 0)
        {
            ret = 0.0 / 0.0;    // NaN
        }
    }
#endif  // !HAVE_COMPATIBLE_LOG10
    LOGEXIT("log10 returns double %f\n", ret);
    return ret;
}

/*++
Function:
    pow

See MSDN.
--*/
PALIMPORT double __cdecl PAL_pow(double x, double y)
{
    double ret;

    ENTRY("pow (x=%f, y=%f)\n", x, y);
#if !HAVE_COMPATIBLE_POW
    if (y == 1.0 / 0.0 && !isnan(x))    // +Inf
    {
        if (x == 1.0 || x == -1.0)
        {
            ret = 0.0 / 0.0;    // NaN
        }
        else if (x > -1.0 && x < 1.0)
        {
            ret = 0.0;
        }
        else
        {
            ret = 1.0 / 0.0;    // +Inf
        }
    }
    else if (y == -1.0 / 0.0 && !isnan(x))   // -Inf
    {
        if (x == 1.0 || x == -1.0)
        {
            ret = 0.0 / 0.0;    // NaN
        }
        else if (x > -1.0 && x < 1.0)
        {
            ret = 1.0 / 0.0;    // +Inf
        }
        else
        {
            ret = 0.0;
        }
    }
    else if (x == 0.0 && y < 0.0)
    {
        ret = 1.0 / 0.0;    // +Inf
    }
    else
#endif  // !HAVE_COMPATIBLE_POW
    if (y == 0.0 && isnan(x))
    {
        // Windows returns NaN for pow(NaN, 0), but POSIX specifies
        // a return value of 1 for that case.  We need to return
        // the same result as Windows.
        ret = 0.0 / 0.0;    // NaN
    }
    else
    {
        ret = pow(x, y);
    }
#if !HAVE_VALID_NEGATIVE_INF_POW
    if (ret == 1.0 / 0.0 && x < 0 && finite(x) && (long long) y % 2 != 0)
    {
        ret = -1.0 / 0.0;   // -Inf
    }
#endif  // !HAVE_VALID_NEGATIVE_INF_POW
    LOGEXIT("pow returns double %f\n", ret);
    return ret;
}
