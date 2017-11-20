/*=============================================================
**
** Source: helper.c
**
** Purpose: Intended to be the child process of a debugger.  Calls 
**          OutputDebugStringA once with a normal string, once with an empty
**          string
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

int __cdecl main(int argc, char *argv[])
{
    if(0 != (PAL_Initialize(argc, argv)))
    {
        return FAIL;
    }

    OutputDebugStringA("Foo!\n");

    OutputDebugStringA("");

    /* give a chance to the debugger process to read the debug string before 
       exiting */
    Sleep(1000);

    PAL_Terminate();
    return PASS;
}
