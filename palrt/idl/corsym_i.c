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

#ifdef _MSC_VER
#pragma warning( disable: 4049 )  /* more than 64k source lines */
#endif

/* this ALWAYS GENERATED file contains the IIDs and CLSIDs */

/* link this file in with the server and any clients */


 /* File created by MIDL compiler version 6.00.0347 */
//@@MIDL_FILE_HEADING(  )


#ifdef __cplusplus
extern "C"{
#endif 


#include <rpc.h>
#include <rpcndr.h>

#ifdef _MIDL_USE_GUIDDEF_

#ifndef INITGUID
#define INITGUID
#include <guiddef.h>
#undef INITGUID
#else
#include <guiddef.h>
#endif

#define MIDL_DEFINE_GUID(type,name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8) \
        DEFINE_GUID(name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8)

#else // !_MIDL_USE_GUIDDEF_

#ifndef __IID_DEFINED__
#define __IID_DEFINED__

typedef struct _IID
{
    unsigned long x;
    unsigned short s1;
    unsigned short s2;
    unsigned char  c[8];
} IID;

#endif // __IID_DEFINED__

#ifndef CLSID_DEFINED
#define CLSID_DEFINED
typedef IID CLSID;
#endif // CLSID_DEFINED

#define MIDL_DEFINE_GUID(type,name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8) \
        const type name = {l,w1,w2,{b1,b2,b3,b4,b5,b6,b7,b8}}

#endif // !_MIDL_USE_GUIDDEF_

MIDL_DEFINE_GUID(IID, LIBID_CorSymLib,0x7E348441,0x7E1F,0x380E,0xA0,0xF6,0x22,0x66,0x8F,0x0F,0x9E,0x4B);


MIDL_DEFINE_GUID(CLSID, CLSID_CorSymWriter,0x108296C1,0x281E,0x11d3,0xBD,0x22,0x00,0x00,0xF8,0x08,0x49,0xBD);


MIDL_DEFINE_GUID(CLSID, CLSID_CorSymReader,0x108296C2,0x281E,0x11d3,0xBD,0x22,0x00,0x00,0xF8,0x08,0x49,0xBD);


MIDL_DEFINE_GUID(CLSID, CLSID_CorSymBinder,0xAA544D41,0x28CB,0x11d3,0xBD,0x22,0x00,0x00,0xF8,0x08,0x49,0xBD);


MIDL_DEFINE_GUID(IID, IID_ISymUnmanagedBinder,0xAA544D42,0x28CB,0x11d3,0xBD,0x22,0x00,0x00,0xF8,0x08,0x49,0xBD);


MIDL_DEFINE_GUID(IID, IID_ISymUnmanagedDocument,0x40DE4037,0x7C81,0x3E1E,0xB0,0x22,0xAE,0x1A,0xBF,0xF2,0xCA,0x08);


MIDL_DEFINE_GUID(IID, IID_ISymUnmanagedDocumentWriter,0xB01FAFEB,0xC450,0x3A4D,0xBE,0xEC,0xB4,0xCE,0xEC,0x01,0xE0,0x06);


MIDL_DEFINE_GUID(IID, IID_ISymUnmanagedMethod,0xB62B923C,0xB500,0x3158,0xA5,0x43,0x24,0xF3,0x07,0xA8,0xB7,0xE1);


MIDL_DEFINE_GUID(IID, IID_ISymUnmanagedNamespace,0x0DFF7289,0x54F8,0x11d3,0xBD,0x28,0x00,0x00,0xF8,0x08,0x49,0xBD);


MIDL_DEFINE_GUID(IID, IID_ISymUnmanagedReader,0xB4CE6286,0x2A6B,0x3712,0xA3,0xB7,0x1E,0xE1,0xDA,0xD4,0x67,0xB5);


MIDL_DEFINE_GUID(IID, IID_ISymUnmanagedScope,0x68005D0F,0xB8E0,0x3B01,0x84,0xD5,0xA1,0x1A,0x94,0x15,0x49,0x42);


MIDL_DEFINE_GUID(IID, IID_ISymUnmanagedVariable,0x9F60EEBE,0x2D9A,0x3F7C,0xBF,0x58,0x80,0xBC,0x99,0x1C,0x60,0xBB);


MIDL_DEFINE_GUID(IID, IID_ISymUnmanagedWriter,0x2DE91396,0x3844,0x3B1D,0x8E,0x91,0x41,0xC2,0x4F,0xD6,0x72,0xEA);

#undef MIDL_DEFINE_GUID

#ifdef __cplusplus
}
#endif





#ifdef _MSC_VER
#pragma warning( disable: 4049 )  /* more than 64k source lines */
#endif

/* this ALWAYS GENERATED file contains the IIDs and CLSIDs */

/* link this file in with the server and any clients */


 /* File created by MIDL compiler version 6.00.0347 */
//@@MIDL_FILE_HEADING(  )


