/*=====================================================================
**
** Source:  test4.c (exception_handling\pal_except\test4)
**
** Purpose: Test to make sure the PAL_EXCEPT block is executed
**          after an exception occurs in the PAL_TRY block
**          if the PAL_EXCEPT block contains a nested 
**          PAL_TRY-PAL_EXCEPT block
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

/* Execution flags */
BOOL bTry           = FALSE;
BOOL bExcept        = FALSE;
BOOL bTry_nested    = FALSE;
BOOL bExcept_nested = FALSE;

int __cdecl main(int argc, char *argv[])
{
    if (0 != PAL_Initialize(argc, argv))
    {
        return FAIL;
    }

    PAL_TRY 
    {
        int* p = 0x00000000;   /* NULL pointer */

        bTry = TRUE;    /* indicate we hit the PAL_TRY block */
        *p = 13;        /* causes an access violation exception */

        Fail("ERROR: code was executed after the access violation.\n");
    }
    PAL_EXCEPT_EX(1, EXCEPTION_EXECUTE_HANDLER)
    {
        if (!bTry)
        {
            Fail("ERROR: PAL_EXCEPT was hit without PAL_TRY being hit.\n");
        }

        PAL_TRY
        {
            int *lp = 0x00000000;

            bTry_nested = TRUE;
            *lp = 13; /* causes an access violation exception */

            Fail("ERROR: code was executed after the "
                 "nested access violation.\n");
        }
        PAL_EXCEPT_EX(2, EXCEPTION_EXECUTE_HANDLER)
        {
            if (!bTry_nested)
            {
                Fail("ERROR: PAL_EXCEPT was hit without PAL_TRY being hit "
                     "in the nested block.\n");
            }
            bExcept_nested = TRUE;
        }
        PAL_ENDTRY;

        bExcept = TRUE; /* indicate we hit the PAL_EXCEPT block */
    }
    PAL_ENDTRY;

    if (!bTry)
    {
        Trace("ERROR: the code in the PAL_TRY block was not executed.\n");
    }

    if (!bExcept)
    {
        Trace("ERROR: the code in the PAL_EXCEPT block was not executed.\n");
    }

    if (!bTry_nested)
    {
        Trace("ERROR: the code in the nested "
              "PAL_TRY block was not executed.\n");
    }

    if (!bExcept_nested)
    {
        Trace("ERROR: the code in the nested "
              "PAL_EXCEPT block was not executed.\n");
    }

    /* did we hit all the code blocks? */
    if(!bTry || !bExcept ||
       !bTry_nested || !bExcept_nested)
    {
        Fail("");
    }


    PAL_Terminate();  
    return PASS;

}
