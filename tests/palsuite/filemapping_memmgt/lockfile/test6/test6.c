/*=============================================================
**
** Source: test6.c
**
** Purpose: 
** Append to a file which is locked until the end of the file, and
** append to a file which is locked past the end of the file.  (The first
** should succeed, while the second should fail)
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
**============================================================*/

#include <palsuite.h>
#include "../lockfile.h"

#define HELPER "helper"
#define FILENAME "testfile.txt"

/* This test checks that you can append to a file which is locked from Start
   to EOF.
*/
void Test1()
{
    HANDLE TheFile = NULL;
    DWORD FileStart = 0;
    DWORD FileEnd = 0;
    int result;
    char* WriteBuffer = "12345678901234567890123456"; 
      
    /* Call the helper function to Create a file, write 'WriteBuffer' to
       the file, and lock the file.
    */
    
    FileEnd = strlen(WriteBuffer);
    TheFile = CreateAndLockFile(TheFile,FILENAME, WriteBuffer, 
                                FileStart, FileEnd);
    
    
    /*
      Launch another proccess which will attempt to append to the 
      end of the file.  Note: This returns -1 if the setup failed in some way.
    */

    result = RunHelper(HELPER);

    if(result == -1)
    {
        Fail("ERROR: The Helper program failed in setting up the "
             "test, so it could never be run.");
    }
    else if(result == 0)
    {
        Fail("ERROR: Failed to append to the file which was Locked from "
             "start until EOF.  Should have been able to append to this "
             "file still.  GetLastError() is %d.",GetLastError());
    }

    if(UnlockFile(TheFile, FileStart, 0, FileEnd, 0) == 0)
    {
        Fail("ERROR: UnlockFile failed.  GetLastError returns %d.",
             GetLastError());
    }
    
    if(CloseHandle(TheFile) == 0)
    {
        Fail("ERROR: CloseHandle failed to close the file. "
             "GetLastError() returned %d.",GetLastError());
    }

}

/* This test checks that you can't append to a file which is locked beyond 
   EOF.
*/
void Test2()
{
    HANDLE TheFile = NULL;
    DWORD FileStart = 0;
    DWORD FileEnd = 0;
    int result;
    char* WriteBuffer = "12345678901234567890123456"; 
    
    /* Call the helper function to Create a file, write 'WriteBuffer' to
       the file, and lock the file.
    */
    
    FileEnd = strlen(WriteBuffer);
    TheFile = CreateAndLockFile(TheFile,FILENAME, WriteBuffer, 
                                FileStart, FileEnd+20);
    
    
    /*
      Launch another proccess which will attempt to append to the 
      end of the file.
    */

    result = RunHelper(HELPER);

    if(result == -1)
    {
        Fail("ERROR: The Helper program failed in setting up the "
             "test, so it could never be run.");
    }
    else if(result > 0)
    {
        Fail("ERROR: The Helper program successfully appended to the "
             "end of the file, even though it was locked beyond EOF.  This "
             "should have failed.");
    }

    if(UnlockFile(TheFile, FileStart, 0, FileEnd+20, 0) == 0)
    {
        Fail("ERROR: UnlockFile failed.  GetLastError returns %d.",
             GetLastError());
    }
    
    if(CloseHandle(TheFile) == 0)
    {
        Fail("ERROR: CloseHandle failed to close the file. "
             "GetLastError() returned %d.",GetLastError());
    }

}


int __cdecl main(int argc, char *argv[])
{
    
    if(0 != (PAL_Initialize(argc, argv)))
    {
        return FAIL;
    }
    
    /* Test a file which is locked until EOF to see if you can append */
    Test1();

    /* Test a file which is locked past EOF to ensure you can't append */
    Test2();
    
    PAL_Terminate();
    return PASS;
}
