/*=====================================================================
**
** Source:  test3.c
**
** Purpose: Tests the PAL implementation of the fread function.
**          Open a file in READ mode, then try to read from the file with
**          different 'size' params.  Check to ensure the return values and
**          the text in the buffer is correct.
**
** Depends:
**     fopen
**     fseek
**     strcmp
**     memset
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

/* Note: testfile should exist in the directory with 15 characters 
   in it ... something got lost if it isn't here.
*/

#include <palsuite.h>

int __cdecl main(int argc, char **argv)
{
    const char filename[] = "testfile";
    char buffer[128];
    FILE * fp = NULL;
    int result;

    if (0 != PAL_Initialize(argc, argv))
    {
        return FAIL;
    }

    /* Open a file in READ mode */

    if((fp = fopen(filename, "r")) == NULL)
    {
        Fail("Unable to open a file for reading.  Is the file "
             "in the directory?  It should be.");
    }

    memset(buffer,'x',128);

    /* Put the null one character past the end of the text that was read
       in, to ensure that it wasn't reading in 0
    */ 

    buffer[16] = '\0';

    /* Attempt to read in 5 bytes at a time.  This should return 3 and 
       contain the full string in the buffer.
    */
    
    if((result = fread(buffer,5,3,fp)) != 3)
    {
        Fail("ERROR: Attempted to read in data of size 5.  The file has "
             "15 bytes in it so 3 items should have been read.  But the value "
             "returned was %d.",result);
    }

    if(strcmp(buffer, "This is a test.x") != 0)
    {
        Fail("ERROR: The buffer should have contained the text "
             "'This is a test.x' but instead contained '%s'.",buffer);
    }

    memset(buffer,'x',128);

    if(fseek(fp, 0, SEEK_SET)) 
    {
        Fail("ERROR: fseek failed, and this test depends on it.");
    }    

    buffer[16] = '\0';

    /* Attempt to read in 6 bytes at a time. The return should be 2.  The
       full string should still be in the buffer.
    */
    
    if((result = fread(buffer,6,3,fp)) != 2)
    {
        Fail("ERROR: Attempted to read in data of size 6.  The file has "
             "15 bytes in it, so 2 items should have been read.  But the "
             "value returned was %d.",result);
    }

    if(strcmp(buffer, "This is a test.x") != 0)
    {
        Fail("ERROR: The buffer should have contained the text "
             "'This is a test.x' but instead contained '%s'.",buffer);
    }

    memset(buffer,'x',128);

    buffer[7] = '\0';

    if(fseek(fp, 0, SEEK_SET)) 
    {
        Fail("ERROR: fseek failed, and this test depends on it.");
    }

    /* Attempt to read in 6 bytes at a time but only one item max. 
       The return should be 1.  The first 6 characters should be in the
       buffer.
    */
    
    if((result = fread(buffer,6,1,fp)) != 1)
    {
        Fail("ERROR: Attempted to read in data of size 6 with a max count "
             "of 1. Thus, one item should have been read, but the "
             "value returned was %d.",result);
    }

    if(strcmp(buffer, "This ix") != 0)
    {
        Fail("ERROR: The buffer should have contained the text "
             "'This ix.' but instead contained '%s'.",buffer);
    }

    PAL_Terminate();
    return PASS;
}


