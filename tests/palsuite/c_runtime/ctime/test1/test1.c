/*=============================================================================
**
** Source: test1.c
**
** Purpose: Test to ensure that ctime return a valid 
**          string when it received a valid number of second.
**          Test to ensure that ctime return null when it
**          receives a invalid number of second.
** 
** Dependencies: PAL_Initialize
**               PAL_Terminate
**				 Fail
**               IsBadReadPtr
**               strcmp
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
**===========================================================================*/

#include <palsuite.h>

/*
 * Date strings generated under win2000/WinXP, times & Strings are GMT
 */
const time_t VAL_SUN_JAN_17_2038   = 2147383647;
const char *STR_SUN_JAN_17_2038    = "Sun Jan 17 23:27:27 2038\n";

/* Note, there are two acceptable strings for this date. */
/* The day can have a leading 0 under Windows.           */
const time_t VAL_FRI_JAN_02_1970   = 100000;  
const char *STR_FRI_JAN_02_1970    = "Fri Jan 02 03:46:40 1970\n";
const char *STR_FRI_JAN__2_1970    = "Fri Jan  2 03:46:40 1970\n";

const int STR_TIME_SIZE =  26;    /* returned date size in byte*/



int __cdecl main(int argc, char **argv)
{  
    time_t LTime;
    char  *DateResult;      
    TIME_ZONE_INFORMATION tzInformation;

    /*
     * Initialize the PAL and return FAILURE if this fails
     */

    if (PAL_Initialize(argc, argv))
    {
       return FAIL;
    }

    // Get the current timezone information
    GetTimeZoneInformation(&tzInformation);

    /*
     * Test #1
     */
    
    /* set the valid date in time_t format, adjusted for current time zone*/
    LTime = VAL_SUN_JAN_17_2038 + (tzInformation.Bias * 60);

    /* convert it to string using ctime*/
    DateResult = ctime( &LTime );

    /* if it's null, ctime failed*/
    if (DateResult == NULL)
    {
        Fail ("ERROR: (Test #1) ctime returned NULL. Expected string\n");
    }
    
    /* test if the entire string can ba access normaly    */
    if(IsBadReadPtr(DateResult, STR_TIME_SIZE)==0)
    {
        /* compare result with win2000 result */
        if(strcmp( DateResult, STR_SUN_JAN_17_2038)!=0)
        {
            Fail("ERROR: (Test #1) ctime returned an unexpected string " 
                 "%s, expexted string is %s\n"
                 ,DateResult, STR_SUN_JAN_17_2038);
        }
    }
    else
    {
        Fail ("ERROR: (Test #1) ctime returned a bad pointer.\n");
    }


    /*
     * Test #2
     */

    /* Set the valid date in time_t format, adjusted for current time zone */
    LTime = VAL_FRI_JAN_02_1970 + (tzInformation.Bias * 60);

    /* convert it to string using ctime   */
    DateResult = ctime( &LTime );

    /* if it's null, ctime failed*/
    if (DateResult == NULL)
    {
        Fail ("ERROR: (Test #2) ctime returned NULL. Expected string\n");
    }   

    /* test if the entire string can ba access normaly    */
    if(IsBadReadPtr(DateResult, STR_TIME_SIZE)==0)
    {
        /* compare result with win2000 result  */
        if (strcmp(DateResult, STR_FRI_JAN_02_1970) != 0
            && strcmp(DateResult, STR_FRI_JAN__2_1970) != 0)
        {
            Fail("ERROR: (Test #2) ctime returned an unexpected string " 
                 "%s, expected string is %s\n"
                 ,DateResult, STR_FRI_JAN_02_1970);
        }
    }
    else
    {
        Fail ("ERROR: (Test #2) ctime returned a bad pointer.\n");
    }       


    
    /*
     * Test #3
     */

    /* specify an invalid time  */
    LTime = -1;

    /* try to convert it        */
    DateResult = ctime( &LTime );    

    /* Check the result for errors, should fail in this case */
    if (DateResult != NULL)
    {
        Fail ("ERROR: (Test #3) ctime returned something different from NULL.:"
              "Expected NULL\n");
    }


    PAL_Terminate();
    return PASS;
}






