/*============================================================================
**
** Source:  test5.c
**
** Purpose: Test #5 for the wcstoul function
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
 * Notes: wcstoul should depend on the current locale's LC_NUMERIC category, 
 * this is not currently tested.
 */


int __cdecl main(int argc, char *argv[])
{
    WCHAR overstr[] = {'4','2','9','4','9','6','7','2','9','6',0};
    WCHAR understr[] = {'-','1',0};
    WCHAR *end;
    unsigned long l;
    
    if (0 != PAL_Initialize(argc, argv))
    {
        return FAIL;
    }


    errno = 0;
    l = wcstoul(overstr, &end, 10);

    if (l != ULONG_MAX)
    {
        Fail("ERROR: Expected wcstoul to return %d, got %d\n", ULONG_MAX, l);
    }
    if (end != overstr + 10)
    {
        Fail("ERROR: Expected wcstoul to give an end value of %p, got %p\n",
            overstr + 10, end);
    }
    if (errno != ERANGE)
    {
        Fail("ERROR: wcstoul did not set errno to ERANGE (%d)\n", errno);
    }

    errno = 0;
    l = wcstoul(understr, &end, 10);

    if (l != ULONG_MAX)
    {
        Fail("ERROR: Expected wcstoul to return %d, got %d\n", ULONG_MAX, l);
    }
    if (end != understr + 2)
    {
        Fail("ERROR: Expected wcstoul to give an end value of %p, got %p\n",
            understr + 2, end);
    }
    if (errno != 0)
    {
        Fail("ERROR: wcstoul set errno to non-zero (%d)\n", errno);
    }
    
    PAL_Terminate();
    return PASS;
}
