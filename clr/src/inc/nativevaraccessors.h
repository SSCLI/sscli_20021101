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
// The following are used to read and write data given NativeVarInfo
// for primitive types. Don't use these for VALUECLASSes.
//*****************************************************************************

#include "corjit.h"

bool operator ==(const ICorDebugInfo::VarLoc &varLoc1,
                 const ICorDebugInfo::VarLoc &varLoc2);

SIZE_T  NativeVarSize(const ICorDebugInfo::VarLoc & varLoc);

DWORD *NativeVarStackAddr(const ICorDebugInfo::VarLoc &   varLoc, 
                        PCONTEXT                        pCtx);
                        
bool    GetNativeVarVal(const ICorDebugInfo::VarLoc &   varLoc, 
                        PCONTEXT                        pCtx,
                        DWORD                       *   pVal1, 
                        DWORD                       *   pVal2);
                        
bool    SetNativeVarVal(const ICorDebugInfo::VarLoc &   varLoc, 
                        PCONTEXT                        pCtx,
                        DWORD                           val1, 
                        DWORD                           val2);                        
