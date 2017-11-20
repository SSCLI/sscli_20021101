/*============================================================================
**
** Source:  test5.c
**
** Purpose: Tests _snwprintf with the count specifier
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
#include "../_snwprintf.h"

/* memcmp is used to verify the results, so this test is dependent on it. */
/* ditto with wcslen */

int __cdecl main(int argc, char *argv[])
{
    WCHAR *longStr;
    WCHAR *longResult;

    if (PAL_Initialize(argc, argv) != 0)
    {
        return FAIL;
    }

    longStr = convert("really-long-string-that-just-keeps-going-on-and-on-and-on.."
        "..................useless-filler.................................."
        "..................useless-filler.................................."
        "..................useless-filler.................................."
        "%n bar");
    longResult = convert("really-long-string-that-just-keeps-going-on-and-on-and-on.."
        "..................useless-filler.................................."
        "..................useless-filler.................................."
        "..................useless-filler.................................."
        " bar");
    DoCountTest(convert("foo %n bar"), 4, convert("foo  bar"));
    DoCountTest(longStr, 257, longResult);
    DoCountTest(convert("fo%n bar"), 2, convert("fo bar"));
    DoCountTest(convert("%n"), 0, convert(""));
    DoCountTest(convert("foo %#n bar"), 4, convert("foo  bar"));
    DoCountTest(convert("foo % n bar"), 4, convert("foo  bar"));
    DoCountTest(convert("foo %+n bar"), 4, convert("foo  bar"));
    DoCountTest(convert("foo %-n bar"), 4, convert("foo  bar"));
    DoCountTest(convert("foo %0n bar"), 4, convert("foo  bar"));
    DoShortCountTest(convert("foo %hn bar"), 4, convert("foo  bar"));
    DoCountTest(convert("foo %ln bar"), 4, convert("foo  bar"));
    DoCountTest(convert("foo %Ln bar"), 4, convert("foo  bar"));
    DoCountTest(convert("foo %I64n bar"), 4, convert("foo  bar"));
    DoCountTest(convert("foo %20.3n bar"), 4, convert("foo  bar"));

    free(longStr);
    free(longResult);

    PAL_Terminate();

    return PASS;
}
