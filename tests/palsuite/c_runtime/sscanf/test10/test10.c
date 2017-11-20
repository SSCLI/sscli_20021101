/*============================================================================
**
** Source:  test10.c
**
** Purpose:  Tests sscanf with wide charactersn
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
#include "../sscanf.h"

int __cdecl main(int argc, char *argv[])
{
    if (PAL_Initialize(argc, argv))
    {
        return FAIL;
    }

    DoWCharTest("1234d", "%C", convert("1"), 1);
    DoWCharTest("1234d", "%C", convert("1"), 1);
    DoWCharTest("abc", "%2C", convert("ab"), 2);
    DoWCharTest(" ab", "%C", convert(" "), 1);
    DoCharTest("ab", "%hC", "a", 1);
    DoWCharTest("ab", "%lC", convert("a"), 1);
    DoWCharTest("ab", "%LC", convert("a"), 1);
    DoWCharTest("ab", "%I64C", convert("a"), 1);

    PAL_Terminate();
    return PASS;
}
