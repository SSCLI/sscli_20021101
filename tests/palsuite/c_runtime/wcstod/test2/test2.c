/*=====================================================================
**
** Source:  test2.c
**
** Purpose: Tests wcstod with overflows
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

int __cdecl main(int argc, char **argv)
{
    /* Representation of positive infinty for a IEEE 64-bit double */
    INT64 PosInifity = (INT64)(0x7ff00000) << 32;    
    double HugeVal = *(double*) &PosInifity;
    char *PosStr = "1E+10000";
    char *NegStr = "-1E+10000";
    WCHAR *wideStr;
    double result;  
  

    if (PAL_Initialize(argc,argv))
    {
        return FAIL;
    }
  
    wideStr = convert(PosStr);
    result = wcstod(wideStr, NULL);    
    free(wideStr);

    if (result != HugeVal)
    {        
        Fail("ERROR: wcstod interpreted \"%s\" as %g instead of %g\n",
            PosStr, result, HugeVal);
    }



    wideStr = convert(NegStr);
    result = wcstod(wideStr, NULL);
    free(wideStr);
    
    if (result != -HugeVal)
    {
        Fail("ERROR: wcstod interpreted \"%s\" as %g instead of %g\n",
            NegStr, result, -HugeVal);
    }


    PAL_Terminate();

    return PASS;
}

