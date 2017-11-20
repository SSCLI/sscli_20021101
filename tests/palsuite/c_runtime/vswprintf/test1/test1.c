/*=====================================================================
**
** Source:    test1.c
**
** Purpose:   Test #1 for the vswprintf function.
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
#include "../vswprintf.h"

/* memcmp is used to verify the results, so this test is dependent on it. */
/* ditto with wcslen */


int __cdecl main(int argc, char *argv[])
{
    WCHAR *checkstr = convert("hello world");
    WCHAR buf[256] = { 0 };

    if (PAL_Initialize(argc, argv) != 0)
        return(FAIL);

    testvswp(buf, checkstr);
    if (memcmp(checkstr, buf, wcslen(checkstr)*2+2) != 0)
    {
        Fail("ERROR: Expected \"%s\", got \"%s\"\n", 
            convertC(checkstr), convertC(buf));
    }

    PAL_Terminate();
    return PASS;
}
