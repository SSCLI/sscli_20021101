/*============================================================================
**
** Source:  test3.c
**
** Purpose: Test #3 for the wcstol function. Tests an invalid string
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


/*
 * Notes: wcstol should depend on the current locale's LC_NUMERIC category, 
 * this is not currently tested.
 */


int __cdecl main(int argc, char *argv[])
{
    WCHAR str[] = {'Z',0};
    WCHAR *end;
    long l;
    
    if (0 != PAL_Initialize(argc, argv))
    {
        return FAIL;
    }


    l = wcstol(str, &end, 10);

    if (l != 0)
    {
        Fail("ERROR: Expected wcstol to return %d, got %d\n", 0, l);
    }

    if (end != str)
    {
        Fail("ERROR: Expected wcstol to give an end value of %p, got %p\n",
            str + 3, end);
    }

    PAL_Terminate();
    return PASS;
}
