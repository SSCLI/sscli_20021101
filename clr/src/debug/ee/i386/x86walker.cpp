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
//*****************************************************************************
// File: x86walker.cpp
//
// x86 instruction decoding/stepping logic
//
//*****************************************************************************

#include "stdafx.h"

#include "walker.h"

#include "frames.h"
#include "openum.h"


#ifdef _X86_

//
// Decode the mod/rm part of an instruction.
//
void NativeWalker::DecodeModRM(BYTE mod, BYTE reg, BYTE rm, const BYTE *ip)
{
    switch (mod)
    {
    case 0:
        {
            if (rm == 5)
            {
                // rm == 5 is a basic disp32
                m_nextIP =
                    (BYTE*) *(*((UINT32**)ip));
                m_skipIP = ip + 4;
            }
            else if (rm == 4)
            {
                // rm == 4 means a SIB follows.
                BYTE sib = *ip++;
                BYTE scale = (sib & 0xC0) >> 6;
                BYTE index = (sib & 0x38) >> 3;
                BYTE base  = (sib & 0x07);

                // grab the index register
                DWORD indexVal = 0;
                            
                if (m_registers != NULL)
                    indexVal = GetRegisterValue(index);

                // scale the index
                indexVal *= 1 << scale;
                            
                // base == 5 indicates a 32 bit displacement
                if (base == 5)
                {
                    // Grab the displacement
                    UINT32 disp = *((UINT32*)ip);

                    // nextIP is [index + disp]...
                    m_nextIP = (BYTE*) *((UINT32*) (indexVal +
                                                    disp));

                    // Make sure to skip the disp.
                    m_skipIP = ip + 4;
                }
                else
                {
                    // nextIP is just [index]
                    m_nextIP = (BYTE*) *((UINT32*) indexVal);
                    m_skipIP = ip;
                }
            }
            else
            {
                // rm == 0, 1, 2, 3, 6, 7 is [register]
                if (m_registers != NULL)
                    m_nextIP = (BYTE*) *((UINT32*) GetRegisterValue(rm));

                m_skipIP = ip;
            }

            break;
        }

    case 1:
        {
            char tmp = *ip; // it's important that tmp is a _signed_ value

            if (m_registers != NULL)
                m_nextIP = (BYTE*) *((UINT32*)(GetRegisterValue(rm) + tmp));

            m_skipIP = ip + 1;

            break;
        }

    case 2:
        {
            /* !!! seems wrong... */
            UINT32 tmp = *(UINT32*)ip;

            if (m_registers != NULL)
                m_nextIP = (BYTE*) *((UINT32*)(GetRegisterValue(rm) + tmp));

            m_skipIP = ip + 4;
            break;
        }
                
    case 3:
        {
            if (m_registers != NULL)
                m_nextIP = (BYTE*) GetRegisterValue(rm);

            m_skipIP = ip;
            break;
        }
                
    default:
        _ASSERTE(!"Invalid mod!");
    }
}

//
// The x86 walker is currently pretty minimal.  It only recognizes call and return opcodes, plus a few jumps.  The rest
// is treated as unknown.
//
void NativeWalker::Decode()
{
    const BYTE *ip = m_ip;

    m_type = WALK_UNKNOWN;
    m_skipIP = NULL;
    m_nextIP = NULL;

    LOG((LF_CORDB, LL_INFO100000, "NW:Decode: m_ip 0x%x\n", m_ip));
    //
    // Skip instruction prefixes
    //
    do 
    {
        switch (*ip)
        {
        // Segment overrides
        case 0x26: // ES
        case 0x2E: // CS
        case 0x36: // SS
        case 0x3E: // DS 
        case 0x64: // FS
        case 0x65: // GS

        // Size overrides
        case 0x66: // Operand-Size
        case 0x67: // Address-Size

        // Lock
        case 0xf0:

        // String REP prefixes
        case 0xf1:
        case 0xf2: // REPNE/REPNZ
        case 0xf3: 
            LOG((LF_CORDB, LL_INFO10000, "NW:Decode: prefix:%0.2x ", *ip));
            ip++;
            continue;

        default:
            break;
        }
    } while (0);

    // Read the opcode
    m_opcode = *ip++;

    LOG((LF_CORDB, LL_INFO100000, "NW:Decode: ip 0x%x, m_opcode:%0.2x\n", ip, m_opcode));

    if (m_opcode == 0xcc)
    {
        m_opcode = DebuggerController::GetPatchedOpcode(m_ip);
        LOG((LF_CORDB, LL_INFO100000, "NW:Decode after patch look up: m_opcode:%0.2x\n", m_opcode));
    }

    // Analyze what we can of the opcode
    switch (m_opcode)
    {
    case 0xff:
        {
            // This doesn't decode all the possible addressing modes of the call instruction, just the ones I know we're
            // using right now. We really need this to decode everything someday...
            BYTE modrm = *ip++;
            BYTE mod = (modrm & 0xC0) >> 6;
            BYTE reg = (modrm & 0x38) >> 3;
            BYTE rm  = (modrm & 0x07);

            LOG((LF_CORDB, LL_INFO100000, "NW:Decode: reg 0x%x\n", reg));
            switch (reg)
            {
            case 2: // reg == 2 indicates that these are the "FF /2" calls (CALL r/m32)
            case 3: // FF /3 - CALL m16:32
                m_type = WALK_CALL;
                DecodeModRM(mod, reg, rm, ip);
                break;

            case 4: // FF /4 -- JMP r/m32
            case 5: // FF /5 -- JMP m16:32
                m_type = WALK_BRANCH;
                m_isAbsoluteBranch = true;
                DecodeModRM(mod, reg, rm, ip);
                break;
                
            default:
                // A call or JMP we don't decode.
                break;
            }

            break;
        }

    case 0xe8: // CALL relative
        {
            m_type = WALK_CALL;

            UINT32 disp = *((UINT32*)ip);
            m_nextIP = ip + 4 + disp;
            m_skipIP = ip + 4;

            break;
        }

    case 0xA2: // CALL absolute
    case 0xC8: // ENTER
        {
            m_type = WALK_CALL;
            m_isAbsoluteBranch = true;

            m_nextIP = (BYTE*) *((UINT32*)ip);
            m_skipIP = ip + 4;
            break;
        }

    case 0x9a: // CALL ptr16:16/32
        {
            m_type = WALK_CALL;

            m_nextIP = (BYTE*) *((UINT32*)ip);
            m_skipIP = ip + 4;

            break;
        }

    case 0xc2: // RET
    case 0xc3: // RET N
        m_isAbsoluteBranch = true;
        m_type = WALK_RETURN;
        break;

    case 0xca: // RET imm16
    case 0xcb: // RET
        m_type = WALK_RETURN;
        break;

    case 0xEA: // JMP far
        m_isAbsoluteBranch = true;
        m_type = WALK_BRANCH;
        break;



    default:
        break;
    }
}


//
// Given a regdisplay and a register number, return the value of the register.
//

DWORD NativeWalker::GetRegisterValue(int registerNumber)
{

    _ASSERTE(m_registers != NULL);
    switch (registerNumber)
    {
    case 0:
        return *m_registers->pEax;
        break;
    case 1:
        return *m_registers->pEcx;
        break;
    case 2:
        return *m_registers->pEdx;
        break;
    case 3:
        return *m_registers->pEbx;
        break;
    case 4:
        return m_registers->SP;
        break;
    case 5:
        return *m_registers->pEbp;
        break;
    case 6:
        return *m_registers->pEsi;
        break;
    case 7:
        return *m_registers->pEdi;
        break;
    default:
        _ASSERTE(!"Invalid register number!");
    }
    
    return 0;
}

#endif
