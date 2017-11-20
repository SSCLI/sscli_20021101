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
// File: walker.h
//
// Debugger code stream analysis routines
//
//*****************************************************************************

#ifndef WALKER_H_
#define WALKER_H_

/* ========================================================================= */

/* ------------------------------------------------------------------------- *
 * Constants
 * ------------------------------------------------------------------------- */

enum WALK_TYPE
{
  WALK_NEXT,
  WALK_BRANCH,
  WALK_COND_BRANCH,
  WALK_CALL,
  WALK_RETURN,
  WALK_BREAK,
  WALK_THROW,
  WALK_META,
  WALK_UNKNOWN
};

/* ------------------------------------------------------------------------- *
 * Classes
 * ------------------------------------------------------------------------- */

class Walker
{
protected:
    Walker()
      : m_opcode(0), m_type(WALK_UNKNOWN), m_registers(NULL), m_ip(0), m_skipIP(0), m_nextIP(0), m_isAbsoluteBranch(false)
      {}

public:

    void Init(const BYTE *ip, REGDISPLAY *pregisters)
    { 
        m_registers = pregisters;
        SetIP(ip);
    }

    const BYTE *GetIP()
      { return m_ip; }

    DWORD GetOpcode()
      { return m_opcode; }

    WALK_TYPE GetOpcodeWalkType()
      { return m_type; }

    const BYTE *GetSkipIP()
      { return m_skipIP; }

    bool IsAbsoluteBranch()
      { return m_isAbsoluteBranch; }


    const BYTE *GetNextIP()
      { return m_nextIP; }

      // We don't currently keep the registers up to date
    virtual void Next() { m_registers = NULL; SetIP(m_nextIP); }
    virtual void Skip() { m_registers = NULL; SetIP(m_skipIP); }

    // Decode the instruction
    virtual void Decode() = 0;

private:
    void SetIP(const BYTE *ip)
      { m_ip = ip; Decode(); }

protected:
    DWORD               m_opcode;           // Current instruction or opcode
    WALK_TYPE           m_type;             // Type of instructions
    REGDISPLAY         *m_registers;        // Registers
    const BYTE         *m_ip;               // Current IP
    const BYTE         *m_skipIP;           // IP if we skip the instruction
    const BYTE         *m_nextIP;           // IP if the instruction is taken
    bool                m_isAbsoluteBranch; // Is it an obsolute branch or not
};

#ifdef _X86_

class NativeWalker : public Walker
{
public:

    void SetRegDisplay(REGDISPLAY *registers)
      { m_registers = registers; }

    REGDISPLAY *GetRegDisplay()
      { return m_registers; }

    void Decode();
    void DecodeModRM(BYTE mod, BYTE reg, BYTE rm, const BYTE *ip);

private:
    DWORD GetRegisterValue(int registerNumber);
};

#elif defined (_PPC_)

class NativeWalker : public Walker
{
public:

    void Decode();
private:
    bool TakeBranch(DWORD instruction);
};
#else
PORTABILITY_WARNING("NativeWalker not implemented on this platform");
class NativeWalker : public Walker
{
public:
    void Next()
      { Walker::Next(); }
    void Skip()
      { Walker::Skip(); }

    void Decode()
    {
        PORTABILITY_ASSERT("NativeWalker not implemented on this platform");
        m_type = WALK_UNKNOWN;
        m_skipIP = m_ip++;
        m_nextIP = m_ip++;        
    }
};
#endif


#endif // WALKER_H_
