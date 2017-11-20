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
// ============================================================================
// File: argslot.h
//
// ============================================================================
// Contains the ARG_SLOT type.

#ifndef __ARG_SLOT_H__
#define __ARG_SLOT_H__

// The ARG_SLOT must be big enough to represent all pointer and basic types (except for 80-bit fp values).
// So, it's guaranteed to be at least 64-bit.
typedef unsigned __int64 ARG_SLOT;

#define PtrToArgSlot(ptr) ((ARG_SLOT)(SIZE_T)(ptr))
#define ArgSlotToPtr(s)   ((LPVOID)(SIZE_T)(s))

// Returns the address of the payload inside the argslot
inline BYTE* ArgSlotEndianessFixup(ARG_SLOT* pArg, UINT cbSize) {
    BYTE* pBuf = (BYTE*)pArg;
#ifdef BIGENDIAN
    switch (cbSize) {
    case 1:
        pBuf += 7;
        break;
    case 2:
        pBuf += 6;
        break;
    case 4:
        pBuf += 4;
        break;
    }
#endif
    return pBuf;
}

#endif  // __ARG_SLOT_H__
