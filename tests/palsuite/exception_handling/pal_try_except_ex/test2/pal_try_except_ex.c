/*=====================================================================
**
** Source:  PAL_TRY_EXCEPT_EX.c (test 2)
**
** Purpose: Tests the PAL implementation of the PAL_TRY and 
**          PAL_EXCEPT_EX functions. Exceptions are not forced to ensure
**          the proper blocks are hit.
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


int __cdecl main(int argc, char *argv[])
{
    BOOL bTry = FALSE;
    BOOL bExcept = FALSE;
    BOOL bTestA = TRUE;

    if (0 != PAL_Initialize(argc, argv))
    {
        return FAIL;
    }


    /*
    ** test to make sure we get into the exception block
    */
    
    PAL_TRY 
    {
        if (!bTestA)
        {
            Fail("PAL_TRY_EXCEPT_EX: ERROR ->"
                " It appears the first try block was hit a second time.\n");
        }
        bTry = TRUE;    /* indicate we hit the PAL_TRY block */
    }
    PAL_EXCEPT_EX(TEST_A, EXCEPTION_EXECUTE_HANDLER)
    {
        bExcept = TRUE; /* indicate we hit the PAL_EXCEPT block */
    }
    PAL_ENDTRY;

    if (!bTry)
    {
        Trace("PAL_TRY_EXCEPT_EX: ERROR -> It appears the code in the PAL_TRY"
            " block was not executed.\n");
    }

    if (bExcept)
    {
        Trace("PAL_TRY_EXCEPT_EX: ERROR -> It appears the code in the first"
            " PAL_EXCEPT block was executed even though no exceptions were "
            "encountered.\n");
    }

    /* did we hit all the proper code blocks? */
    if(!bTry || bExcept)
    {
        Fail("");
    }


    /*
    ** test to make sure we skip the second exception block
    */
    
    bTry = FALSE;
    bExcept = FALSE;
    bTestA = FALSE;    /* we are now going into the second block test */


    PAL_TRY 
    {
        if (bTestA)
        {
            Fail("PAL_TRY_EXCEPT_EX: ERROR -> It appears"
                " the second try block was hit too early.\n");
        }
        bTry = TRUE;    /* indicate we hit the PAL_TRY block */
    }
    PAL_EXCEPT_EX(TEST_B, EXCEPTION_EXECUTE_HANDLER)
    {
        if (bTestA)
        {
            Fail("PAL_TRY_EXCEPT_EX: ERROR -> It appears"
                " the second except block was hit too early.\n");
        }
        bExcept = TRUE; /* indicate we hit the PAL_TRY block */
    }
    PAL_ENDTRY;

    if (!bTry)
    {
        Trace("PAL_TRY_EXCEPT_EX: ERROR -> It appears the code in the second"
            " PAL_TRY block was not executed.\n");
    }

    if (bExcept)
    {
        Trace("PAL_TRY_EXCEPT_EX: ERROR -> It appears the code in the second"
            " PAL_EXCEPT block was executed even though no exceptions were "
            "encountered.\n");
    }

    /* did we hit all the proper code blocks? */
    if(!bTry || bExcept)
    {
        Fail("\n");
    }

    PAL_Terminate();  
    return PASS;

}
