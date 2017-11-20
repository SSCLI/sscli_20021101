/*=====================================================================
**
** Source:  GetSystemTimeAsFileTime.c
**
** Purpose: Tests the PAL implementation of GetSystemTimeAsFileTime
** Take two times, three seconds apart, and ensure that the time is 
** increasing, and that it has increased at least 3 seconds.
**
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

    FILETIME TheFirstTime, TheSecondTime;
    ULONG64 FullFirstTime, FullSecondTime;

    if (0 != PAL_Initialize(argc,argv))
    {
        return FAIL;
    }
  
    /* Get two times, 3 seconds apart */

    GetSystemTimeAsFileTime( &TheFirstTime );

    Sleep( 3000 );
  
    GetSystemTimeAsFileTime( &TheSecondTime );

    /* Convert them to ULONG64 to work with */

    FullFirstTime = ((( (ULONG64)TheFirstTime.dwHighDateTime )<<32) | 
                     ( (ULONG64)TheFirstTime.dwLowDateTime ));
  
    FullSecondTime = ((( (ULONG64)TheSecondTime.dwHighDateTime )<<32) | 
                      ( (ULONG64)TheSecondTime.dwLowDateTime ));

    /* Test to ensure the second value is larger than the first */

    if( FullSecondTime <= FullFirstTime )
    {
        Fail("ERROR:  The system time didn't increase in the last "
               "three seconds.  The second time tested was less than "
               "or equal to the first.");
    }

    /* Note: This magic number is 3 seconds in hundreds of nano
       seconds.  This test checks to ensure at least 3 seconds passed
       between the readings.
    */
  
    if( ( FullSecondTime - FullFirstTime ) < 30000000 )
    {
        Fail("ERROR: Two system times were tested, with a sleep of 3 "
               "seconds between.  The time passed should have been at least "
               "3 seconds.  But, it was less according to the function.");
    }

    PAL_Terminate();
    return PASS;
}

