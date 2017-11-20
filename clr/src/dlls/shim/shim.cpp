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
// Shim.cpp
//
//*****************************************************************************
#include "stdafx.h"                     // Standard header.


extern "C" INT32 __stdcall ND_RU1(VOID *psrc, INT32 ofs)
{
    return (INT32) *( (UINT8*)(ofs + (BYTE*)psrc) );
}

extern "C" INT32 __stdcall ND_RI2(VOID *psrc, INT32 ofs)
{
    return (INT32) *( (INT16*)(ofs + (BYTE*)psrc) );
}

extern "C" INT32 __stdcall ND_RI4(VOID *psrc, INT32 ofs)
{
    return (INT32) *( (INT32*)(ofs + (BYTE*)psrc) );
}

extern "C" INT64 __stdcall ND_RI8(VOID *psrc, INT32 ofs)
{
    return (INT64) *( (INT64*)(ofs + (BYTE*)psrc) );
}

extern "C" VOID __stdcall ND_WU1(VOID *psrc, INT32 ofs, UINT8 val)
{
    *( (UINT8*)(ofs + (BYTE*)psrc) ) = val;
}

extern "C" VOID __stdcall ND_WI2(VOID *psrc, INT32 ofs, INT16 val)
{
    *( (INT16*)(ofs + (BYTE*)psrc) ) = val;
}

extern "C" VOID __stdcall ND_WI4(VOID *psrc, INT32 ofs, INT32 val)
{
    *( (INT32*)(ofs + (BYTE*)psrc) ) = val;
}

extern "C" VOID __stdcall ND_WI8(VOID *psrc, INT32 ofs, INT64 val)
{
    *( (INT64*)(ofs + (BYTE*)psrc) ) = val;
}

extern "C" VOID __stdcall ND_CopyObjSrc(LPBYTE source, int ofs, LPBYTE dst, int cb)
{
    CopyMemory(dst, source + ofs, cb);
}

extern "C" VOID __stdcall ND_CopyObjDst(LPBYTE source, LPBYTE dst, int ofs, int cb)
{
    CopyMemory(dst + ofs, source, cb);
}

