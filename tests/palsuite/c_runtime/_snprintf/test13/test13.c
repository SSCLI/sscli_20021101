/*============================================================================
**
** Source:  test13.c
**
** Purpose: Tests _snprintf with hex numbers (uppercase)
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
#include "../_snprintf.h"

/*
 * Notes: memcmp is used, as is strlen.
 */


int __cdecl main(int argc, char *argv[])
{
    int neg = -42;
    int pos = 0x1234AB;
    INT64 l = 0x1234567887654321;
    
    if (PAL_Initialize(argc, argv) != 0)
    {
        return FAIL;
    }


    DoNumTest("foo %X", pos, "foo 1234AB");
    DoNumTest("foo %lX", pos, "foo 1234AB");
    DoNumTest("foo %hX", pos, "foo 34AB");
    DoNumTest("foo %LX", pos, "foo 1234AB");
    DoI64Test("foo %I64X", l, "0x1234567887654321", "foo 1234567887654321");
    DoNumTest("foo %7X", pos, "foo  1234AB");
    DoNumTest("foo %-7X", pos, "foo 1234AB ");
    DoNumTest("foo %.1X", pos, "foo 1234AB");
    DoNumTest("foo %.7X", pos, "foo 01234AB");
    DoNumTest("foo %07X", pos, "foo 01234AB");
    DoNumTest("foo %#X", pos, "foo 0X1234AB");
    DoNumTest("foo %+X", pos, "foo 1234AB");
    DoNumTest("foo % X", pos, "foo 1234AB");
    DoNumTest("foo %+X", neg, "foo FFFFFFD6");
    DoNumTest("foo % X", neg, "foo FFFFFFD6");

    PAL_Terminate();
    return PASS;
}

