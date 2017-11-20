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
// ===========================================================================
// File: menuids.h
//
// ===========================================================================

//////////////////////////////////////////////////////////////////////////////
//
// Guids defined and used by C#
//
//////////////////////////////////////////////////////////////////////////////
#define NOICON  guidOfficeIcon:msotcidNoIcon

#ifdef DEFINE_GUID

// guid for C# package clsid
// {A066E284-DCAB-11d2-B551-00C04F68D4DB}
    DEFINE_GUID (CLSID_MSSCPackage, 0xa066e284, 0xdcab, 0x11d2, 0xb5, 0x51, 0x0, 0xc0, 0x4f, 0x68, 0xd4, 0xdb);

//  guid for C# commands
// {D91AF2F7-61F6-4d90-BE23-D057D2EA961B}
    DEFINE_GUID (guidCSharpCmdId, 0xd91af2f7, 0x61f6, 0x4d90, 0xbe, 0x23, 0xd0, 0x57, 0xd2, 0xea, 0x96, 0x1b);

// guid for C# groups and menus
// {5D7E7F65-A63F-46ee-84F1-990B2CAB23F9}
    DEFINE_GUID (guidCSharpGrpId, 0x5d7e7f65, 0xa63f, 0x46ee, 0x84, 0xf1, 0x99, 0xb, 0x2c, 0xab, 0x23, 0xf9);

#else

// guid for C# package clsid
    #define CLSID_MSSCPackage    { 0xa066e284, 0xdcab, 0x11d2, { 0xb5, 0x51, 0x0, 0xc0, 0x4f, 0x68, 0xd4, 0xdb } }

// guid for C# commands
    #define guidCSharpCmdId      { 0xd91af2f7, 0x61f6, 0x4d90, { 0xbe, 0x23, 0xd0, 0x57, 0xd2, 0xea, 0x96, 0x1b } }

// guid for C# groups and menus
    #define guidCSharpGrpId      { 0x5d7e7f65, 0xa63f, 0x46ee, { 0x84, 0xf1, 0x99, 0xb, 0x2c, 0xab, 0x23, 0xf9 } }

#endif



// Groups
#define IDG_CSHARP_CV_WIZARDS    0x03F2  // ClassView wizards grp for C# - added to the default CV context menu

//Command IDs
#define icmdCSharpClassWiz                0x0000
#define icmdCSharpIndexerWiz              0x0001
#define icmdCSharpInterfaceIndexerWiz     0x0002
#define icmdCSharpInterfaceMethodWiz      0x0003
#define icmdCSharpInterfacePropWiz        0x0004
#define icmdCSharpMethodWiz               0x0005
#define icmdCSharpMemVariableWiz          0x0006
#define icmdCSharpPropWiz                 0x0007

    
