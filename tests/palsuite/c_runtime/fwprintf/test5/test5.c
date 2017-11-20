/*============================================================================
**
** Source:      test5.c
**
** Purpose:     Tests the count specifier (%n).
**              This test is modeled after the sprintf series.
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
#include "../fwprintf.h"

/* 
 * Depends on memcmp, strlen, fopen, fseek and fgets.
 */

int __cdecl main(int argc, char *argv[])
{    
    WCHAR *longStr;
    char *longResult = 
        "really-long-string-that-just-keeps-going-on-and-on-and-on.."
        "..................useless-filler.................................."
        "..................useless-filler.................................."
        "..................useless-filler.................................."
        " bar";

    if (PAL_Initialize(argc, argv))
    {
        return FAIL;
    }

    longStr = 
        convert("really-long-string-that-just-keeps-going-on-and-on-and-on.."
        "..................useless-filler.................................."
        "..................useless-filler.................................."
        "..................useless-filler.................................."
        "%n bar");
    DoCountTest(convert("foo %n bar"), 4, "foo  bar");
    DoCountTest(longStr, 257, longResult);
    DoCountTest(convert("fo%n bar"), 2, "fo bar");
    DoCountTest(convert("%n foo"), 0, " foo");
    DoCountTest(convert("foo %#n bar"), 4, "foo  bar");
    DoCountTest(convert("foo % n bar"), 4, "foo  bar");
    DoCountTest(convert("foo %+n bar"), 4, "foo  bar");
    DoCountTest(convert("foo %-n bar"), 4, "foo  bar");
    DoCountTest(convert("foo %0n bar"), 4, "foo  bar");
    DoShortCountTest(convert("foo %hn bar"), 4, "foo  bar");
    DoCountTest(convert("foo %ln bar"), 4, "foo  bar");
    DoCountTest(convert("foo %Ln bar"), 4, "foo  bar");
    DoCountTest(convert("foo %I64n bar"), 4, "foo  bar");
    DoCountTest(convert("foo %20.3n bar"), 4, "foo  bar");

    PAL_Terminate();

    free(longStr);

    return PASS;
}
