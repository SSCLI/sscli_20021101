/*=====================================================================
**
** Source:    test1.c
**
** Purpose:   Test #1 for the vsprintf function.
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
#include "../vsprintf.h"

/*
 * Notes: memcmp is used, as is strlen.
 */

int __cdecl main(int argc, char *argv[])
{
    char checkstr[] = "hello world";
    char buf[256] = { 0 };
    int ret;
    
    if (PAL_Initialize(argc, argv) != 0)
    {
        return(FAIL);
    }

    testvsp(buf, "hello world");

    if (memcmp(checkstr, buf, strlen(checkstr)+1) != 0)
    {
        Fail("ERROR: expected \"%s\" (up to %d chars), got \"%s\"\n",
             checkstr, 256, buf);
    }

    testvsp(buf, "xxxxxxxxxxxxxxxxx");
    ret = testvsp(buf,  "hello world");

    if (ret != strlen(checkstr))
    {
        Fail("ERROR: expected negative return value, got %d", ret);
    }

    if (memcmp(checkstr, buf, ret) != 0)
    {
        Fail("ERROR: expected %s, got %s\n", checkstr, buf);
    }

    PAL_Terminate();
    return PASS;
}
