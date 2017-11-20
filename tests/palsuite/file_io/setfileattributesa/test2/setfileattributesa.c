/*=====================================================================
**
** Source:  SetFileAttributesA.c
**
**
** Purpose: Tests the PAL implementation of the SetFileAttributesA function
** Test that we can set the defined attributes aside from READONLY on a
** file, and that it doesn't return failure.  Note, these attributes won't
** do anything to the file, however.
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
    DWORD TheResult;
    char* FileName = {"test_file"};
    
    if (0 != PAL_Initialize(argc,argv))
    {
        return FAIL;
    }
    

    
    /* Try to set the file to HIDDEN */

    TheResult = SetFileAttributesA(FileName, FILE_ATTRIBUTE_HIDDEN);
    
    if(TheResult == 0)
    {
        Fail("ERROR: SetFileAttributesA returned 0, failure, when trying "
               "to set the FILE_ATTRIBUTE_HIDDEN attribute.  This should "
               "not do anything in FreeBSD, but it shouldn't fail.");
    }

    /* Try to set the file to ARCHIVE */

    TheResult = SetFileAttributesA(FileName, FILE_ATTRIBUTE_ARCHIVE);
    
    if(TheResult == 0)
    {
        Fail("ERROR: SetFileAttributesA returned 0, failure, when trying "
               "to set the FILE_ATTRIBUTE_ARCHIVE attribute.");
    }

    /* Try to set the file to SYSTEM */

    TheResult = SetFileAttributesA(FileName, FILE_ATTRIBUTE_SYSTEM);
    
    if(TheResult == 0)
    {
        Fail("ERROR: SetFileAttributesA returned 0, failure, when trying "
               "to set the FILE_ATTRIBUTE_SYSTEM attribute.");
    }

    /* Try to set the file to DIRECTORY */

    TheResult = SetFileAttributesA(FileName, FILE_ATTRIBUTE_DIRECTORY);
    
    if(TheResult == 0)
    {
        Fail("ERROR: SetFileAttributesA returned 0, failure, when trying "
               "to set the FILE_ATTRIBUTE_DIRECTORY attribute.");
    }
    
    PAL_Terminate();
    return PASS;
}
