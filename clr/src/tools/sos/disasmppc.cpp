// ==++==
// 
//   
//    Copyright (c) 2002 Microsoft Corporation.  All rights reserved.
//   
//    The use and distribution terms for this software are contained in the file
//    named license.txt, which can be found in the root of this distribution.
//    By using this software in any fashion, you are agreeing to be bound by the
//    terms of this license.
//   
//    You must not remove this notice, or any other, from this software.
//   
// 
// ==--==

#include "strike.h"
#include "eestructs.h"
#include "util.h"
#include "disasm.h"
#ifndef PLATFORM_UNIX
#include <dbghelp.h>
#endif

void DumpStackObjectsHelper (size_t StackTop, size_t StackBottom)
{
    DWORD_PTR ptr = StackTop & ~3;  // make certain dword aligned

    for (;ptr < StackBottom; ptr += sizeof(DWORD_PTR))
    {
        if (IsInterrupt())
            return;
        DWORD_PTR objAddr;
        move (objAddr, ptr);
        DWORD_PTR mtAddr;
        if (SUCCEEDED(g_ExtData->ReadVirtual((ULONG64)objAddr, &mtAddr, sizeof(mtAddr), NULL))) {
            if (IsMethodTable(mtAddr)) {
                ExtOut ("%p %p ", (ULONG64)ptr, (ULONG64)objAddr);
                NameForMT (mtAddr,g_mdName);
                ExtOut ("%S", g_mdName);
                if (mtAddr == MTForString()) {
                    ExtOut ("    ");
                    StringObjectContent(objAddr, FALSE, 40);
                }
                ExtOut ("\n");
            }
        }
    }
}
