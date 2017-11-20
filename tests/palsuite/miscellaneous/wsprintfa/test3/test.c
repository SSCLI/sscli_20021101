/*============================================================
**
** Source: test.c
**
** Purpose: Test for wsprintfA() function
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
**=========================================================*/

#include <palsuite.h>

/* memcmp is used to verify the results, so this test is dependent on it. */
/* ditto with strlen */

char * ErrorMessage; 
char buf[256];

BOOL test1()
{

    /* Test 1 */
    wsprintf(buf, "foo %S", convert("bar"));
    if (memcmp(buf, "foo bar", strlen(buf) + 1) != 0) 
    {
        ErrorMessage = "ERROR: (Test 1) Failed. The correct string is "
            "'foo bar' and the result returned was ";
        return FAIL;
    }

    /* Test 2 */
    wsprintf(buf, "foo %hS", "bar");
    if (memcmp(buf, "foo bar", strlen(buf) + 1) != 0) 
    {
        ErrorMessage = "ERROR: (Test 2) Failed. The correct string is "
            "'foo bar' and the result returned was ";
        return FAIL;
    }

    /* Test 3 */
    wsprintf(buf, "foo %lS", convert("bar"));
    if (memcmp(buf, "foo bar", strlen(buf) + 1) != 0) 
    {
        ErrorMessage = "ERROR: (Test 3) Failed. The correct string is '"
            "foo bar' and the result returned was ";
        return FAIL;
    }


    /* Test 4 */
    wsprintf(buf, "foo %5S", convert("bar"));
    if (memcmp(buf, "foo   bar", strlen(buf) + 1) != 0)
    {
        ErrorMessage = "ERROR: (Test 4) Failed. The correct string is "
            "'foo   bar' and the result returned was ";
        return FAIL;
    }

    /* Test 5 */
    wsprintf(buf, "foo %.2S", convert("bar"));
    if (memcmp(buf, "foo ba", strlen(buf) + 1) != 0)
    {
        ErrorMessage = "ERROR: (Test 5) Failed. The correct string is "
            "'foo ba' and the result returned was ";
        return FAIL;
    }

    /* Test 6 */
    wsprintf(buf, "foo %5.2S", convert("bar"));
    if (memcmp(buf, "foo    ba", strlen(buf) + 1) != 0)
    {
        ErrorMessage = "ERROR: (Test 6) Failed. The correct string is "
            "'foo    ba' and the result returned was ";
        return FAIL;
    }

    /* Test 7 */
    wsprintf(buf, "foo %-5S", convert("bar"));
    if (memcmp(buf, "foo bar  ", strlen(buf) + 1) != 0)
    {
        ErrorMessage = "ERROR: (Test 7) Failed. The correct string is "
            "'foo bar  ' and the result returned was ";
        return FAIL;
    }

    /* Test 8 */
    wsprintf(buf, "foo %05S", convert("bar"));
    if (memcmp(buf, "foo 00bar", strlen(buf) + 1) != 0) {
        ErrorMessage = "ERROR: (Test 8) Failed. The correct string is "
            "'foo 00bar' and the result returned was ";
        return FAIL;
    }
    return PASS;
}

int __cdecl main(int argc, char *argv[])
{

    /*
     * Initialize the PAL and return FAILURE if this fails
     */

    if(0 != (PAL_Initialize(argc, argv)))
    {
        return FAIL;
    }

    if(test1()) {
        Fail("%s '%s'\n",ErrorMessage,buf);

    }

    PAL_Terminate();
    return PASS;

}


