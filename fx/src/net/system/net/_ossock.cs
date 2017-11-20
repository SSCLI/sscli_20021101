//------------------------------------------------------------------------------
// <copyright file="_OSSOCK.cs" company="Microsoft">
//     
//      Copyright (c) 2002 Microsoft Corporation.  All rights reserved.
//     
//      The use and distribution terms for this software are contained in the file
//      named license.txt, which can be found in the root of this distribution.
//      By using this software in any fashion, you are agreeing to be bound by the
//      terms of this license.
//     
//      You must not remove this notice, or any other, from this software.
//     
// </copyright>
//------------------------------------------------------------------------------

namespace System.Net {
    using System.Net.Sockets;
    using System.Runtime.InteropServices;

    //
    // Argument structure for IP_ADD_MEMBERSHIP and IP_DROP_MEMBERSHIP.
    //
    [StructLayout(LayoutKind.Sequential)]
    internal struct IPMulticastRequest {

        public int MulticastAddress; // IP multicast address of group
        public int InterfaceAddress; // local IP address of interface

        public static readonly int Size = Marshal.SizeOf(typeof(IPMulticastRequest));
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct Linger {

        public short OnOff; // option on/off
        public short Time; // linger time

        public static readonly int Size = Marshal.SizeOf(typeof(Linger));
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct WSABuffer {

        public int Length; // Length of Buffer
        public IntPtr Pointer;// Pointer to Buffer

    } // struct WSABuffer

    [StructLayout(LayoutKind.Sequential)]
    internal struct WSAData {
        public short wVersion;
        public short wHighVersion;
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst=257)]
        public string szDescription;
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst=129)]
        public string szSystemStatus;
        public short iMaxSockets;
        public short iMaxUdpDg;
        public int lpVendorInfo;
    }



    //
    // used as last parameter to WSASocket call
    //
    [Flags]
    internal enum SocketConstructorFlags {
        WSA_FLAG_OVERLAPPED         = 0x01,
        WSA_FLAG_MULTIPOINT_C_ROOT  = 0x02,
        WSA_FLAG_MULTIPOINT_C_LEAF  = 0x04,
        WSA_FLAG_MULTIPOINT_D_ROOT  = 0x08,
        WSA_FLAG_MULTIPOINT_D_LEAF  = 0x10,
    }


} // namespace System.Net
