/*============================================================================
**
** Source:  test1.c
**
** Purpose: Test that memmove correctly copies text from one buffer
**          to another even when the buffers overlap.
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

int __cdecl main(int argc, char **argv)
{
    char testA[11] = "abcdefghij";
    char testB[15] = "aabbccddeeffgg";
    char testC[15] = "aabbccddeeffgg";
    char testD[15] = "aabbccddeeffgg";
    char insString[3] = "zzz";
    char *retVal;

    if (PAL_Initialize(argc, argv))
    {
        return FAIL;
    }

    /* move a string onto itself */
    retVal = (char *)memmove(testA + 2, testA, 8);
    if (retVal != testA + 2)
    {
        Fail("The return value should have been the value of the destination"
             "pointer, but wasn't\n");
    }

    /*Check the most likely error*/
    if (memcmp(testA, "ababababab", 11) == 0)
    {
        Fail("memmove should have saved the characters in the region of"
             " overlap between source and destination, but didn't.\n");
    }

    if (memcmp(testA, "ababcdefgh", 11) != 0)
    {
        /* not sure what exactly went wrong. */
        Fail("memmove was called on a region containing the characters"
             " \"abcdefghij\".  It was to move the first 8 positions to"
             " the last 8 positions, giving the result \"ababcdefgh\". "
             " Instead, it gave the result \"%s\".\n", testA);
    }

    /* move a string to the front of testB */
    retVal = (char *)memmove(testB, insString, 3);
    if(retVal != testB)
    {
        Fail("memmove: The function did not return the correct "
        "string.\n");
    }

    if(memcmp(testB, "zzzbccddeeffgg",15) != 0)
    {
        Fail("memmove: The function failed to move the string "
        "correctly.\n");
    }


    /* move a string to the middle of testC */
    retVal = memmove(testC+5, insString, 3);
    if(retVal != testC+5)
    {
        Fail("memmove: The function did not return the correct "
        "string.\n");
    }

    if(memcmp(testC, "aabbczzzeeffgg",15) != 0)
    {
        Fail("memmove: The function failed to move the string "
        "correctly.\n");
    }


    /* move a string to the end of testD */
    retVal = memmove(testD+11, insString, 3);
    if(retVal != testD+11)
    {
        Fail("memmove: The function did not return the correct "
        "string.\n");
    }

    if(memcmp(testD, "aabbccddeefzzz",15) != 0)
    {
        Fail("memmove: The function failed to move the string "
        "correctly.\n");
    }

    PAL_Terminate();
    return PASS;

}














