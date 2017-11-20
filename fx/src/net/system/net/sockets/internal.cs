//------------------------------------------------------------------------------
// <copyright file="Internal.cs" company="Microsoft">
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

namespace System.Net.Sockets {
    using System;
    using System.Collections;
    using System.Configuration;
    using System.Configuration.Assemblies;
    using System.Diagnostics;
    using System.Globalization;
    using System.IO;
    using System.Net;
    using System.Net.Sockets;
    using System.Reflection;
    using System.Reflection.Emit;
    using System.Resources;
    using System.Runtime.InteropServices;
    using System.Runtime.Serialization;
    using System.Runtime.Serialization.Formatters;
    using System.Security;
    using System.Security.Permissions;
    using System.Security.Util;
    using System.Text;
    using System.Text.RegularExpressions;
    using System.Threading;


    //
    // IO-Control operations are not directly exposed.
    // blocking is controlled by "Blocking" property on socket (FIONBIO)
    // amount of data available is queried by "Available" property (FIONREAD)
    // The other flags are not exposed currently
    //
    internal class IoctlSocketConstants {

        public const int FIONREAD   = 0x4004667F;
        public const int FIONBIO    = unchecked((int)0x8004667E);
        public const int FIOASYNC   = unchecked((int)0x8004667D);

        //
        // not likely to block (sync IO ok)
        //
        // FIONBIO
        // FIONREAD
        // SIOCATMARK
        // SIO_RCVALL
        // SIO_RCVALL_MCAST
        // SIO_RCVALL_IGMPMCAST
        // SIO_KEEPALIVE_VALS
        // SIO_ASSOCIATE_HANDLE (opcode setting: I, T==1)
        // SIO_ENABLE_CIRCULAR_QUEUEING (opcode setting: V, T==1)
        // SIO_GET_BROADCAST_ADDRESS (opcode setting: O, T==1)
        // SIO_GET_EXTENSION_FUNCTION_POINTER (opcode setting: O, I, T==1)
        // SIO_MULTIPOINT_LOOPBACK (opcode setting: I, T==1)
        // SIO_MULTICAST_SCOPE (opcode setting: I, T==1)
        // SIO_TRANSLATE_HANDLE (opcode setting: I, O, T==1)
        // SIO_ROUTING_INTERFACE_QUERY (opcode setting: I, O, T==1)
        //
        // likely to block (reccommended for async IO)
        //
        // SIO_FIND_ROUTE (opcode setting: O, T==1)
        // SIO_FLUSH (opcode setting: V, T==1)
        // SIO_GET_QOS (opcode setting: O, T==1)
        // SIO_GET_GROUP_QOS (opcode setting: O, I, T==1)
        // SIO_SET_QOS (opcode setting: I, T==1)
        // SIO_SET_GROUP_QOS (opcode setting: I, T==1)
        // SIO_ROUTING_INTERFACE_CHANGE (opcode setting: I, T==1)
        // SIO_ADDRESS_LIST_CHANGE (opcode setting: T==1)
    }

    //
    // WinSock 2 extension -- bit values and indices for FD_XXX network events
    //
    [Flags]
    internal enum AsyncEventBits {
        FdNone                     = 0,
        FdRead                     = 1 << 0,
        FdWrite                    = 1 << 1,
        FdOob                      = 1 << 2,    
        FdAccept                   = 1 << 3,
        FdConnect                  = 1 << 4,
        FdClose                    = 1 << 5,
        FdQos                      = 1 << 6,
        FdGroupQos                 = 1 << 7,
        FdRoutingInterfaceChange   = 1 << 8,
        FdAddressListChange        = 1 << 9,
        FdAllEvents                = (1 << 10) - 1,
    }


    [StructLayout(LayoutKind.Sequential)]
    internal struct FileDescriptorSet {
        //
        // how many are set?
        //
        public int Count;
        //
        // an array of Socket handles
        //
        [MarshalAs(UnmanagedType.ByValArray, SizeConst=MaxCount)]
        public IntPtr[] Array; 

        public static readonly int Size = Marshal.SizeOf(typeof(FileDescriptorSet));
        public static readonly FileDescriptorSet Empty = new FileDescriptorSet(0);
        public const int MaxCount = 64;

        public FileDescriptorSet(int count) {
            Count = count;
            Array = count == 0 ? null : new IntPtr[MaxCount];
        }


    } // class FileDescriptorSet

    //
    // Structure used in select() call, taken from the BSD file sys/time.h.
    //
    [StructLayout(LayoutKind.Sequential)]
    internal struct TimeValue {
        public int Seconds;  // seconds
        public int Microseconds; // and microseconds

    } // struct TimeValue



} // namespace System.Net.Sockets
