/*=====================================================================
**
** Source:    test4.c
**
** Purpose:   Test #4 for the vswprintf function.
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
static void DoPointerTest(WCHAR *formatstr, void* param, WCHAR* paramstr,
                   WCHAR *checkstr1)
{
    WCHAR buf[256] = { 0 };

    testvswp(buf, formatstr, param);
    if (memcmp(buf, checkstr1, wcslen(checkstr1) + 2) != 0)

    {
        Fail("ERROR: failed to insert pointer to %#p into \"%s\"\n"
            "Expected \"%s\" got \"%s\".\n",
                paramstr,
                convertC(formatstr), 
                convertC(checkstr1), 
                convertC(buf));
    }    
}

static void DoI64DoubleTest(WCHAR *formatstr, INT64 value, WCHAR *valuestr,
                            WCHAR *checkstr1)
{
    WCHAR buf[256] = { 0 };
 
    testvswp(buf, formatstr, value);
    if (memcmp(buf, checkstr1, wcslen(checkstr1) + 2) != 0)
    {
        Fail("ERROR: failed to insert %s into \"%s\"\n"
            "Expected \"%s\", got \"%s\".\n",
                value,
                convertC(formatstr),
                convertC(checkstr1),
                convertC(buf));
    }
}

int __cdecl main(int argc, char *argv[])
{
    void *ptr = (void*) 0x123456;
    INT64 lptr = 0x1234567887654321;
    
    if (PAL_Initialize(argc, argv) != 0)
        return(FAIL);

    DoPointerTest(convert("%p"), NULL, convert("NULL"), convert("00000000"));
    DoPointerTest(convert("%p"), ptr, convert("pointer to 0x123456"), 
        convert("00123456"));
    DoPointerTest(convert("%9p"), ptr, convert("pointer to 0x123456"), 
        convert(" 00123456"));
    DoPointerTest(convert("%09p"), ptr, convert("pointer to 0x123456"), 
        convert(" 00123456"));
    DoPointerTest(convert("%-9p"), ptr, convert("pointer to 0x123456"), 
        convert("00123456 "));
    DoPointerTest(convert("%+p"), ptr, convert("pointer to 0x123456"), 
        convert("00123456"));
    DoPointerTest(convert("%#p"), ptr, convert("pointer to 0x123456"), 
        convert("0X00123456"));
    DoPointerTest(convert("%lp"), ptr, convert("pointer to 0x123456"), 
        convert("00123456"));
    DoPointerTest(convert("%hp"), ptr, convert("pointer to 0x123456"), 
        convert("00003456"));
    DoPointerTest(convert("%Lp"), ptr, convert("pointer to 0x123456"), 
        convert("00123456"));
    DoI64DoubleTest(convert("%I64p"), lptr, 
        convert("pointer to 0x1234567887654321"), convert("1234567887654321"));

    PAL_Terminate();
    return PASS;
}

