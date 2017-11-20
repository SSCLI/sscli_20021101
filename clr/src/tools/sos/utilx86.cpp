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
#include "data.h"
#include "eestructs.h"
#include "util.h"


/**********************************************************************\
* Routine Description:                                                 *
*                                                                      *
*    Find the begin and end of the code for a managed function.        *  
*                                                                      *
\**********************************************************************/
void CodeInfoForMethodDesc (MethodDesc &MD, CodeInfo &infoHdr, BOOL bSimple)
{
    infoHdr.IPBegin = 0;
    infoHdr.methodSize = 0;
    
    size_t ip = MD.m_CodeOrIL;

    // for EJit and Profiler, m_CodeOrIL has the address of a stub
    unsigned char ch;
    move (ch, ip);
    if (ch == 0xe9)
    {
        int offsetValue;
        move (offsetValue, ip + 1);
        ip = ip + 5 + offsetValue;
    }
    
    DWORD_PTR methodDesc;
    IP2MethodDesc (ip, methodDesc, infoHdr.jitType, infoHdr.gcinfoAddr);
    if (!methodDesc || infoHdr.jitType == UNKNOWN)
    {
        dprintf ("Not jitted code\n");
        return;
    }

    if (infoHdr.jitType == EJIT)
    {
        JittedMethodInfo jittedMethodInfo;
        move (jittedMethodInfo, MD.m_CodeOrIL);
        BYTE* pEhGcInfo = jittedMethodInfo.u2.pEhGcInfo;
        if ((unsigned)pEhGcInfo & 1)
            pEhGcInfo = (BYTE*) ((unsigned) pEhGcInfo & ~1);       // lose the mark bit
        else    // code has not been pitched, and it is guaranteed to not be pitched while we are here
        {
            CodeHeader* pCodeHeader = jittedMethodInfo.u1.pCodeHeader;
            move (pEhGcInfo, pCodeHeader);
        }
        
        if (jittedMethodInfo.flags.EHInfoExists)
        {
            short cEHbytes;
            move (cEHbytes, pEhGcInfo);
            pEhGcInfo = (pEhGcInfo + cEHbytes);
        }
        Fjit_hdrInfo hdrInfo;
        DWORD_PTR dwAddr = (DWORD_PTR)pEhGcInfo;
        hdrInfo.Fill(dwAddr);
        infoHdr.methodSize = (unsigned)hdrInfo.methodSize;
        if (!bSimple)
        {
            infoHdr.prologSize = hdrInfo.prologSize;
            infoHdr.epilogStart = (unsigned char)(infoHdr.methodSize - hdrInfo.epilogSize);
            infoHdr.epilogCount = 1;
            infoHdr.epilogAtEnd = 1;
            infoHdr.ediSaved = 1;
            infoHdr.esiSaved = 1;
            infoHdr.ebxSaved = 1;
            infoHdr.ebpSaved = 1;
            infoHdr.ebpFrame = 1;
            infoHdr.argCount = hdrInfo.methodArgsSize;
        }
    }
    
    infoHdr.IPBegin = ip;
}





/**********************************************************************\
* Routine Description:                                                 *
*                                                                      *
*    This function is called to determine if a DWORD on the stack is   *  
*    a return address.
*    It does this by checking several bytes before the DWORD to see if *
*    there is a call instruction.                                      *
*                                                                      *
\**********************************************************************/
void isRetAddr(DWORD_PTR retAddr, DWORD_PTR* whereCalled)
{
    *whereCalled = 0;
    // don't waste time values clearly out of range
    if (retAddr < 0x1000 || retAddr > 0x80000000)   
        return;

    unsigned char spotend[6];
    move (spotend, retAddr-6);
    unsigned char *spot = spotend+6;
    DWORD_PTR addr;
    
    // Note this is possible to be spoofed, but pretty unlikely
    // call XXXXXXXX
    if (spot[-5] == 0xE8) {
        move (*whereCalled, retAddr-4);
        *whereCalled += retAddr;
        //*whereCalled = *((int*) (retAddr-4)) + retAddr;
        if (*whereCalled < 0x80000000 && *whereCalled > 0x1000)
            return;
        else
            *whereCalled = 0;
    }

    // call [XXXXXXXX]
    if (spot[-6] == 0xFF && (spot[-5] == 025))  {
        move (addr, retAddr-4);
        move (*whereCalled, addr);
        //*whereCalled = **((unsigned**) (retAddr-4));
        if (*whereCalled < 0x80000000 && *whereCalled > 0x1000)
            return;
        else
            *whereCalled = 0;
    }

    // call [REG+XX]
    if (spot[-3] == 0xFF && (spot[-2] & ~7) == 0120 && (spot[-2] & 7) != 4)
    {
        *whereCalled = 0xFFFFFFFF;
        return;
    }
    if (spot[-4] == 0xFF && spot[-3] == 0124)
    {
        *whereCalled = 0xFFFFFFFF;
        return;
    }

    // call [REG+XXXX]
    if (spot[-6] == 0xFF && (spot[-5] & ~7) == 0220 && (spot[-5] & 7) != 4)
    {
        *whereCalled = 0xFFFFFFFF;
        return;
    }
    if (spot[-7] == 0xFF && spot[-6] == 0224)
    {
        *whereCalled = 0xFFFFFFFF;
        return;
    }
    
    // call [REG]
    if (spot[-2] == 0xFF && (spot[-1] & ~7) == 0020 && (spot[-1] & 7) != 4 && (spot[-1] & 7) != 5)
    {
        *whereCalled = 0xFFFFFFFF;
        return;
    }
    
    // call REG
    if (spot[-2] == 0xFF && (spot[-1] & ~7) == 0320 && (spot[-1] & 7) != 4)
    {
        *whereCalled = 0xFFFFFFFF;
        return;
    }
    
    // There are other cases, but I don't believe they are used.
    return;
}
