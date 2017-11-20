/*=============================================================
**
** Source:	_ui64tow.c
**
** Purpose:	Tests _ui64tow with normal values and different
**			radices,highest	and lowest values.
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
**============================================================*/

#include <palsuite.h>

typedef	struct
{
    unsigned __int64    value;
    int	                radix;
    char*               result;
} testCase;


int	__cdecl main(int argc, char	*argv[])
{
    WCHAR   buffer[256];
    WCHAR   *testStr;
    WCHAR   *ret;
    int	    i;
    testCase testCases[] = 
    {
        /* test	limits */
        {0xFFFFFFFFFFFFFFFF, 2,	
            "1111111111111111111111111111111111111111111111111111111111111111"},
        {0xFFFFFFFFFFFFFFFF, 8,	"1777777777777777777777"},
        {0xFFFFFFFFFFFFFFFF, 10, "18446744073709551615"},
        {0xFFFFFFFFFFFFFFFF, 16, "ffffffffffffffff"},
        {47, 2,	"101111"},
        {47, 8,	"57"},
        {47, 10, "47"},
        {47, 16, "2f"},
        {12, 2,	"1100"},
        {12, 8,	"14"},
        {12, 10, "12"},
        {12, 16, "c"},

        /* test with	0. */
        {0,	2, "0"},
        {0,	8, "0"},
        {0,	10,	"0"},
        {0,	16,	"0"}
    };

    if (0 != (PAL_Initialize(argc, argv)))
    {
        return FAIL;
    }

    for	(i=0; i<sizeof(testCases) / sizeof(testCase); i++)
    {
        ret = _ui64tow(testCases[i].value, buffer, testCases[i].radix);

        if (ret	!= buffer)
        {
            Fail("Failed to call _ui64tow API: did not return a	pointer	"
                 "to string. Expected %p, got %p\n", buffer, ret);
        }

        testStr	= convert(testCases[i].result);

        if (wcscmp(testStr, buffer) != 0)
        {	
            Trace("ERROR: _ui64tow test#%d. Expected <%S>, got <%S>.\n",
                   i,testStr, buffer);
            free(testStr);
            Fail("");
        }

        free(testStr);
    }

    PAL_Terminate();
    return PASS;
}



