/*============================================================================
**
** Source:  test4.c
**
** Purpose: Test #4 for the vprintf function. Tests the pointer
**          specifier (%p).
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
#include "../vprintf.h"




int __cdecl main(int argc, char *argv[])
{
    void *ptr = (void*) 0x123456;
    INT64 lptr = 0x1234567887654321;

    if (PAL_Initialize(argc, argv))
    {
        return FAIL;
    }


    DoPointerTest("%p", NULL, "NULL", "00000000");
    DoPointerTest("%p", ptr, "pointer to 0x123456", "00123456");
    DoPointerTest("%9p", ptr, "pointer to 0x123456", " 00123456");
    DoPointerTest("%09p", ptr, "pointer to 0x123456", " 00123456");
    DoPointerTest("%-9p", ptr, "pointer to 0x123456", "00123456 ");
    DoPointerTest("%+p", ptr, "pointer to 0x123456", "00123456");
    DoPointerTest("%#p", ptr, "pointer to 0x123456", "0X00123456");
    DoPointerTest("%lp", ptr, "pointer to 0x123456", "00123456");
    DoPointerTest("%hp", ptr, "pointer to 0x123456", "00003456");
    DoPointerTest("%Lp", ptr, "pointer to 0x123456", "00123456");
    DoI64Test("%I64p", lptr, "pointer to 0x1234567887654321", 
        "1234567887654321");

    PAL_Terminate();
    return PASS;
}

