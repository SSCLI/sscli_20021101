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
    unsigned stubInstrs[4];
    move (stubInstrs, ip);
    if ((stubInstrs[0] == 0x7D8B6378) &&
        (stubInstrs[1] == 0x818C0010) &&
        (stubInstrs[2] == 0x7D8903A6) &&
        (stubInstrs[3] == 0x4E800420))
    {
        move (ip, ip + 4*sizeof(unsigned));
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
            infoHdr.argCount = hdrInfo.methodArgsSize;
        }
    }
    
    infoHdr.IPBegin = ip;
}
