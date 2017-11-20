/*============================================================================
**
** Source:  test1.c
**
** Purpose: Test #1 for the vprintf function. A single, basic, test
**          case with no formatting.
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
    char checkstr[] = "hello world";
    int ret;


    if (PAL_Initialize(argc, argv))
    {
        return FAIL;
    }

    ret = vprintf("hello world", NULL);

    if (ret != strlen(checkstr))
    {
        Fail("Expected vprintf to return %d, got %d.\n", 
            strlen(checkstr), ret);

    }

    PAL_Terminate();
    return PASS;
}

