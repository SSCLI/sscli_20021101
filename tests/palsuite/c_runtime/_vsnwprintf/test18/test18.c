/*=====================================================================
**
** Source:    test18.c
**
** Purpose:   Test #18 for the _vsnwprintf function.
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
**===================================================================*/

#include <palsuite.h>
#include "../_vsnwprintf.h"

/* memcmp is used to verify the results, so this test is dependent on it. */
/* ditto with wcslen */

int __cdecl main(int argc, char *argv[])
{
    double val = 2560.001;
    double neg = -2560.001;
    
    if (PAL_Initialize(argc, argv) != 0)
    {
        return(FAIL);
    }

    DoDoubleTest(convert("foo %G"), val, convert("foo 2560"),
        convert("foo 2560"));
    DoDoubleTest(convert("foo %lG"), val, convert("foo 2560"),
        convert("foo 2560"));
    DoDoubleTest(convert("foo %hG"), val, convert("foo 2560"),
        convert("foo 2560"));
    DoDoubleTest(convert("foo %LG"), val, convert("foo 2560"),
        convert("foo 2560"));
    DoDoubleTest(convert("foo %I64G"), val, convert("foo 2560"),
        convert("foo 2560"));
    DoDoubleTest(convert("foo %5G"), val, convert("foo  2560"),
        convert("foo  2560"));
    DoDoubleTest(convert("foo %-5G"), val, convert("foo 2560 "),
        convert("foo 2560 "));
    DoDoubleTest(convert("foo %.1G"), val, convert("foo 3E+003"),
        convert("foo 3E+03"));
    DoDoubleTest(convert("foo %.2G"), val, convert("foo 2.6E+003"),
        convert("foo 2.6E+03"));
    DoDoubleTest(convert("foo %.12G"), val, convert("foo 2560.001"),
        convert("foo 2560.001"));
    DoDoubleTest(convert("foo %06G"), val, convert("foo 002560"),
        convert("foo 002560"));
    DoDoubleTest(convert("foo %#G"), val, convert("foo 2560.00"),
        convert("foo 2560.00"));
    DoDoubleTest(convert("foo %+G"), val, convert("foo +2560"),
        convert("foo +2560"));
    DoDoubleTest(convert("foo % G"), val, convert("foo  2560"),
        convert("foo  2560"));
    DoDoubleTest(convert("foo %+G"), neg, convert("foo -2560"),
        convert("foo -2560"));
    DoDoubleTest(convert("foo % G"), neg, convert("foo -2560"),
        convert("foo -2560"));

    PAL_Terminate();
    return PASS;
}
