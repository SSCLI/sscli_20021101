/*============================================================================
**
** Source:  test1.c
**
** Purpose: Test #1 for the vfprintf function. A single, basic, test
**          case with no formatting.
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
#include "../vfprintf.h"

int __cdecl main(int argc, char *argv[])
{
    FILE *fp;
    char testfile[] = "testfile.txt";
    char buf[256];
    char checkstr[] = "hello world";
    int ret;


    if (PAL_Initialize(argc, argv))
    {
        return FAIL;
    }

    if ((fp = fopen(testfile, "w+")) == NULL)
    {
        Fail("ERROR: fopen failed to create \"%s\"\n", testfile);
    }

    ret = DoVfprintf(fp, "hello world");

    if (ret != strlen(checkstr))
    {
        Fail("Expected vfprintf to return %d, got %d.\n", 
            strlen(checkstr), ret);

    }

    if ((fseek(fp, 0, SEEK_SET)) != 0)
    {
         Fail("ERROR: Fseek failed to set pointer to beginning of file\n" );
    }
 
    if ((fgets(buf, 100, fp)) == NULL)
    {
        Fail("ERROR: fgets failed\n");
    }    
    if (memcmp(checkstr, buf, strlen(checkstr)+1) != 0)
    {
        Fail("ERROR: expected %s, got %s\n", checkstr, buf);
    }
    if ((fclose(fp)) != 0)
    {
        Fail("ERROR: fclose failed to close \"%s\"\n", testfile);
    }

    PAL_Terminate();
    return PASS;
}
