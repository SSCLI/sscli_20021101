/*============================================================================
**
** Source:  test1.c
**
** Purpose: Test that errno begins as 0, and sets to ERANGE when that 
** error is forced with wcstoul.  
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

int __cdecl main(int argc, char *argv[])
{
    WCHAR overstr[] = {'4','2','9','4','9','6','7','2','9','6',0};
    WCHAR *end;
    
    if (PAL_Initialize(argc, argv))
    {
        return FAIL;
    }
    
    /* 
       From rotor.doc:  The only value that must be supported is 
       ERANGE, in the event that wcstoul() fails due to overflow. 
    */ 
    
    wcstoul(overstr, &end, 10);
    
    if (errno != ERANGE)
    {
        Fail("ERROR: wcstoul did not set errno to ERANGE.  Instead "
             "the value of errno is %d\n", errno);
    }

        
    PAL_Terminate();
    return PASS;
}
