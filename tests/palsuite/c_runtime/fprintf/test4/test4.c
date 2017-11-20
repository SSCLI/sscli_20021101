/*============================================================================
**
** Source:      test4.c (fprintf)
**
** Purpose:     Tests the pointer specifier (%p).
**              This test is modeled after the fprintf series.
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
#include "../fprintf.h"

/* 
 * Depends on memcmp, strlen, fopen, fseek and fgets.
 */

static void DoTest(char *formatstr, void* param, char* paramstr, 
                   char *checkstr1, char *checkstr2)
{
    FILE *fp;
    char buf[256] = { 0 };

    if ((fp = fopen("testfile.txt", "w+")) == NULL )
    {
        Fail("ERROR: fopen failed to create testfile\n");
    }

    if ((fprintf(fp, formatstr, param)) < 0)
    {
        Fail("ERROR: fprintf failed\n");
    }

    if ((fseek(fp, 0, SEEK_SET)) != 0)
    {
        Fail("ERROR: fseek failed\n");
    }

    if ((fgets(buf, 100, fp)) == NULL)
    {
        Fail("ERROR: fseek failed\n");
    }
   
    if (memcmp(buf, checkstr1, strlen(buf) + 1) != 0 &&
        memcmp(buf, checkstr2, strlen(buf) + 1) != 0 )
    {
        Fail("ERROR: failed to insert %s into \"%s\"\n"
            "Expected \"%s\" or \"%s\" got \"%s\".\n", 
            paramstr, formatstr, checkstr1, checkstr2, buf);
    }    
    
    if ((fclose( fp )) != 0)

    {

        Fail("ERROR: fclose failed to close \"testfile.txt\"\n");

    }
}


int __cdecl main(int argc, char *argv[])
{
    void *ptr = (void*) 0x123456;
    INT64 lptr = 0x1234567887654321;

    if (PAL_Initialize(argc, argv) != 0)
        return(FAIL);

    DoTest("%p", NULL, "NULL", "00000000", "0x0");
    DoTest("%p", ptr, "pointer to 0x123456", "00123456", "0x123456");
    DoTest("%9p", ptr, "pointer to 0x123456", " 00123456", " 0x123456");
    DoTest("%09p", ptr, "pointer to 0x123456", " 00123456", "0x0123456");
    DoTest("%-9p", ptr, "pointer to 0x123456", "00123456 ", "0x123456 ");
    DoTest("%+p", ptr, "pointer to 0x123456", "00123456", "0x123456");
    DoTest("%#p", ptr, "pointer to 0x123456", "0X00123456", "0x123456");
    DoTest("%lp", ptr, "pointer to 0x123456", "00123456", "0x123456");
    DoTest("%hp", ptr, "pointer to 0x123456", "00003456", "0x3456");
    DoTest("%Lp", ptr, "pointer to 0x123456", "00123456", "0x123456");
    DoI64Test("%I64p", lptr, "pointer to 0x1234567887654321", 
        "1234567887654321", "0x1234567887654321");

    PAL_Terminate();
    return PASS;
}
