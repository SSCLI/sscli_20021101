/*============================================================================
**
** Source:  test19.c
**
** Purpose: Test #19 for the sprintf function. Tests the variable length
**          precision argument.
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
#include "../sprintf.h"

/* 
 * Depends on memcmp and strlen
 */


int __cdecl main(int argc, char *argv[])
{
    int n = -1;

    if (PAL_Initialize(argc, argv) != 0)
    {
        return FAIL;
    }

    DoArgumentPrecTest("%.*s", 2, "bar", "bar", "ba", "ba");
    DoArgumentPrecTest("%.*S", 2, convert("bar"), "bar", "ba", "ba");

    DoArgumentPrecTest("%.*n", 3, &n, "pointer to int", "", "");
    if (n != 0)
    {
        Fail("ERROR: Expected count parameter to resolve to %d, got %X\n",
              0, n);
    }

    DoArgumentPrecTest("%.*c", 0, (void*)'a', "a", "a", "a");
    DoArgumentPrecTest("%.*c", 4, (void*)'a', "a", "a", "a");
    DoArgumentPrecTest("%.*C", 0, (void*)'a', "a", "a", "a");
    DoArgumentPrecTest("%.*C", 4, (void*)'a', "a", "a", "a");
    DoArgumentPrecTest("%.*d", 1, (void*)42, "42", "42", "42");
    DoArgumentPrecTest("%.*d", 3, (void*)42, "42", "042", "042");
    DoArgumentPrecTest("%.*i", 1, (void*)42, "42", "42", "42");
    DoArgumentPrecTest("%.*i", 3, (void*)42, "42", "042", "042");
    DoArgumentPrecTest("%.*o", 1, (void*)42, "42", "52", "52");
    DoArgumentPrecTest("%.*o", 3, (void*)42, "42", "052", "052");
    DoArgumentPrecTest("%.*u", 1, (void*)42, "42", "42", "42");
    DoArgumentPrecTest("%.*u", 3, (void*)42, "42", "042", "042");
    DoArgumentPrecTest("%.*x", 1, (void*)0x42, "0x42", "42", "42");
    DoArgumentPrecTest("%.*x", 3, (void*)0x42, "0x42", "042", "042");
    DoArgumentPrecTest("%.*X", 1, (void*)0x42, "0x42", "42", "42");
    DoArgumentPrecTest("%.*X", 3, (void*)0x42, "0x42", "042", "042");


    DoArgumentPrecDoubleTest("%.*e", 1, 2.01, "2.0e+000", "2.0e+00");
    DoArgumentPrecDoubleTest("%.*e", 3, 2.01, "2.010e+000", "2.010e+00");
    DoArgumentPrecDoubleTest("%.*E", 1, 2.01, "2.0E+000", "2.0E+00");
    DoArgumentPrecDoubleTest("%.*E", 3, 2.01, "2.010E+000", "2.010E+00");
    DoArgumentPrecDoubleTest("%.*f", 1, 2.01, "2.0", "2.0");
    DoArgumentPrecDoubleTest("%.*f", 3, 2.01, "2.010", "2.010");
    DoArgumentPrecDoubleTest("%.*g", 1, 256.01, "3e+002", "3e+02");
    DoArgumentPrecDoubleTest("%.*g", 3, 256.01, "256", "256");
    DoArgumentPrecDoubleTest("%.*g", 4, 256.01, "256", "256");
    DoArgumentPrecDoubleTest("%.*g", 6, 256.01, "256.01", "256.01");
    DoArgumentPrecDoubleTest("%.*G", 1, 256.01, "3E+002", "3E+02");
    DoArgumentPrecDoubleTest("%.*G", 3, 256.01, "256", "256");
    DoArgumentPrecDoubleTest("%.*G", 4, 256.01, "256", "256");
    DoArgumentPrecDoubleTest("%.*G", 6, 256.01, "256.01", "256.01");

    PAL_Terminate();
    return PASS;
}
