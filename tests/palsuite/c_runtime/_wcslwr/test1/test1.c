/*============================================================================
**
** Source:  test1.c
**
** Purpose: Using memcmp to check the result, convert a wide character string 
** with capitals, to all lowercase using this function. Test #1 for the 
** wcslwr function
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

/* uses memcmp,wcslen */

int __cdecl main(int argc, char *argv[])
{
    WCHAR *string = convert("aSdF 1#");
    WCHAR *checkstr = convert("asdf 1#");
    WCHAR *result = NULL;

    /*
     *  Initialize the PAL and return FAIL if this fails
     */
    if (0 != (PAL_Initialize(argc, argv)))
    {
        return FAIL;
    }

    result = _wcslwr(string);
    if (memcmp(result, checkstr, wcslen(checkstr)*2 + 2) != 0)
    {
        Fail ("ERROR: Expected to get \"%s\", got \"%s\".\n",
                convertC(result), convertC(checkstr));
    }


    PAL_Terminate();
    return PASS;
}

