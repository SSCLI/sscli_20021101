/*=====================================================================
**
** Source:  test1 (PAL_GetBottommostRegistration)
**
** Purpose: Tests the PAL implementation of the PAL_GetBottommostRegistration.
**          The test will call PAL_GetBottommostRegistration in a variety
**          of places within a PAL_TRY, and verify the contents. This test is
**          BSD specific and is pretty much commented out for Win32.
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

const int nConstant = 12321;

LONG ExitFilter()
{
    /* Simple Exception filter, just returns a value.
    */
    return EXCEPTION_EXECUTE_HANDLER;
}

LONG ExitFilter2()
{
    /* Simple Exception filter, just returns a value.
    */
    return EXCEPTION_EXECUTE_HANDLER;
}

LONG ExitFilter3()
{
    /* Simple Exception filter, just returns a value.
    */
    return EXCEPTION_EXECUTE_HANDLER;
}


int __cdecl main(int argc, char *argv[])
{
/*
**
** this is commented out for win32 because it is  
** only available under BSD
**
*/
#ifndef WIN32
    PAL_EXCEPTION_REGISTRATION *pFrame1;
    PAL_EXCEPTION_REGISTRATION *pFrame2;
    PAL_EXCEPTION_REGISTRATION *pFrame3;
    PAL_EXCEPTION_REGISTRATION *pFrame4;

    /* Initalize the PAL.
    */
    if (0 != PAL_Initialize(argc, argv))
    {
        return (FAIL);
    }


    /* Start PAL_TRY to get */
    PAL_TRY 
    {
        pFrame1 = PAL_GetBottommostRegistration();
        PAL_TRY 
        {
            pFrame2 = PAL_GetBottommostRegistration();
            /*Test to see of both frames are the same.
            */
            if(pFrame2 == pFrame1)
            {
                Fail("ERROR: PAL_GetBottommostRegistration retrieved the "
                       "same value both time it ran.\n");
            }

            /* Test to see if pFrame2's next pointer is pFrame1.
            */
            if(pFrame2->Next != pFrame1)
            {
                Fail("ERROR: pFrame2->Next does not point to pFrame1.\n");
            }

            /* See if PAL_EXCEPT_FILTER_EX passes the correct information.
            */
            if(pFrame2->Handler != (PFN_PAL_EXCEPTION_FILTER)ExitFilter2)
            {
                Fail("ERROR: pFrame->Handler does not contain "
                       "\"ExitFilter3\"\n");
            }
        }
        PAL_EXCEPT_FILTER_EX(test2, ExitFilter2, (PVOID)nConstant)
        {
        }
        PAL_ENDTRY;
        
        PAL_TRY 
        {
            pFrame3 = PAL_GetBottommostRegistration();
            /*Test to see of both frames are the same.
            */
            if(pFrame3 == pFrame1)
            {
                Fail("ERROR: PAL_GetBottommostRegistration retrieved the "
                       "same value both time it ran.\n");
            }

            /* Test to see if pFrame3's next pointer is pFrame1.
            */
            if(pFrame3->Next != pFrame1)
            {
                Fail("ERROR: pFrame3->Next does not point to pFrame2.\n");
            }

            /* See if PAL_EXCEPT_FILTER_EX passes the correct information.
            */
            if(pFrame3->Handler != (PFN_PAL_EXCEPTION_FILTER)ExitFilter3)
            {
                Fail("ERROR: pFrame->Handler does not contain "
                       "\"ExitFilter3\"\n");
            }
        }
        PAL_EXCEPT_FILTER_EX(test3, ExitFilter3, (PVOID)nConstant)
        {
        }
        PAL_ENDTRY;

        pFrame4 = PAL_GetBottommostRegistration();
        /* Test to see if pFrame1 and pFrame4 are the same.
        */
        if(pFrame4 != pFrame1)
        {
            Fail("ERROR: PAL_GetBottommostRegistration did not retrieved "
                    "the same value for pFrame4 and pFrame1\n");
        }

        /* See if PAL_EXCEPT_FILTER_EX passes the correct information.
        */
        if(pFrame4->Handler != (PFN_PAL_EXCEPTION_FILTER)ExitFilter)
        {
            Fail("ERROR: pFrame->Handler does not contain "
                    "\"ExitFilter\"\n");
        }

    }
    PAL_EXCEPT_FILTER_EX(test1, ExitFilter, (PVOID)nConstant)
    {
    }
    PAL_ENDTRY;

    PAL_Terminate();  
#endif  // BSD-only test

    return PASS;

}
