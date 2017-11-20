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
// MLGEN.CPP -
//
// Stub generator for ML opcodes.

#include "common.h"
#include "vars.hpp"
#include "ml.h"
#include "stublink.h"
#include "excep.h"
#include "mlgen.h"



//--------------------------------------------------------------
// Emit an opcode.
//--------------------------------------------------------------
VOID MLStubLinker::MLEmit(MLCode opcode)
{
    THROWSCOMPLUSEXCEPTION();
    Emit8(opcode);
}


//--------------------------------------------------------------
// Emit "cb" bytes of uninitialized space.
//--------------------------------------------------------------
VOID MLStubLinker::MLEmitSpace(UINT cb)
{
    THROWSCOMPLUSEXCEPTION();
    while (cb--)
    {
        Emit8(0);
    }
}


//--------------------------------------------------------------
// Reserves "numBytes" bytes of local space and returns the
// offset of the allocated space. Local slots are guaranteed
// to be allocated in increasing order starting from 0. This
// allows ML instructions to use the LOCALWALK ML register
// to implicitly address the locals, rather than burning up
// memory to store a local offset directly in the ML stream.
//--------------------------------------------------------------
UINT16 MLStubLinker::MLNewLocal(UINT16 numBytes)
{
    THROWSCOMPLUSEXCEPTION();

    numBytes = (numBytes + 3) & ~3;
    UINT16 newLocal = m_nextFreeLocal;
    m_nextFreeLocal += numBytes;
    if ( m_nextFreeLocal < newLocal ) {
        COMPlusThrow(kTypeLoadException, IDS_EE_OUTOFLOCALS);
    }
    return newLocal;
}
