//------------------------------------------------------------------------------
// <copyright file="UnsafeNativeMethods.cs" company="Microsoft">
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
    using System.Runtime.InteropServices;
    using System.Security.Permissions;
    using System.Text;
    using System.Net.Sockets;

    [
    System.Security.SuppressUnmanagedCodeSecurityAttribute()
    ]
    internal class UnsafeNclNativeMethods {

 #if !PLATFORM_UNIX
        internal const String DLLPREFIX = "";
        internal const String DLLSUFFIX = ".dll";
 #else // !PLATFORM_UNIX
  #if __APPLE__
        internal const String DLLPREFIX = "lib";
        internal const String DLLSUFFIX = ".dylib";
  #else
        internal const String DLLPREFIX = "lib";
        internal const String DLLSUFFIX = ".so";
  #endif
 #endif // !PLATFORM_UNIX

        internal const String ROTOR_PAL   = DLLPREFIX + "rotor_pal" + DLLSUFFIX;
        internal const String ROTOR_PALRT = DLLPREFIX + "rotor_palrt" + DLLSUFFIX;


        [DllImport(ROTOR_PALRT, CharSet=CharSet.Unicode, SetLastError=true, EntryPoint="PAL_FetchConfigurationStringW")]
        internal static extern bool FetchConfigurationString(bool perMachine, String parameterName, StringBuilder parameterValue, int parameterValueLength);

        //
        // UnsafeNclNativeMethods.OSSOCK class contains all Unsafe() calls and should all be protected
        // by the appropriate SocketPermission() to connect/accept to/from remote
        // peers over the network and to perform name resolution.
        // te following calls deal mainly with:
        // 1) socket calls
        // 2) DNS calls
        //

        //
        // here's a brief explanation of all possible decorations we use for PInvoke.
        //
        // [In] (Note: this is similar to what msdn will show)
        // the managed data will be marshalled so that the unmanaged function can read it but even
        // if it is changed in unmanaged world, the changes won't be propagated to the managed data
        //
        // [Out] (Note: this is similar to what msdn will show)
        // the managed data will not be marshalled so that the unmanaged function will not see the
        // managed data, if the data changes in unmanaged world, these changes will be propagated by
        // the marshaller to the managed data
        //
        // objects are marshalled differently if they're:
        //
        // 1) structs
        // for structs, by default, the whole layout is pushed on the stack as it is.
        // in order to pass a pointer to the managed layout, we need to specify either the ref or out keyword.
        //
        //      a) for IN and OUT:
        //      [In, Out] ref Struct ([In, Out] is optional here)
        //
        //      b) for IN only (the managed data will be marshalled so that the unmanaged
        //      function can read it but even if it changes it the change won't be propagated
        //      to the managed struct)
        //      [In] ref Struct
        //
        //      c) for OUT only (the managed data will not be marshalled so that the
        //      unmanaged function cannot read, the changes done in unmanaged code will be
        //      propagated to the managed struct)
        //      [Out] out Struct ([Out] is optional here)
        //
        // 2) array or classes
        // for array or classes, by default, a pointer to the managed layout is passed.
        // we don't need to specify neither the ref nor the out keyword.
        //
        //      a) for IN and OUT:
        //      [In, Out] byte[]
        //
        //      b) for IN only (the managed data will be marshalled so that the unmanaged
        //      function can read it but even if it changes it the change won't be propagated
        //      to the managed struct)
        //      [In] byte[] ([In] is optional here)
        //
        //      c) for OUT only (the managed data will not be marshalled so that the
        //      unmanaged function cannot read, the changes done in unmanaged code will be
        //      propagated to the managed struct)
        //      [Out] byte[]
        //
        [
        System.Security.SuppressUnmanagedCodeSecurityAttribute()
        ]
        internal class OSSOCK {

            private const string WS2_32 = ROTOR_PAL;

            [DllImport(WS2_32, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern int WSAStartup(
                                               [In] short wVersionRequested,
                                               [Out] out WSAData lpWSAData
                                               );

            [DllImport(WS2_32, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern int WSACleanup();

            [DllImport(WS2_32, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern IntPtr socket(
                                                [In] AddressFamily addressFamily,
                                                [In] SocketType socketType,
                                                [In] ProtocolType protocolType
                                               );

            [DllImport(WS2_32, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern int ioctlsocket(
                                                [In] IntPtr socketHandle,
                                                [In] int cmd,
                                                [In, Out] ref int argp
                                                );

            [DllImport(WS2_32, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern IntPtr gethostbyname(
                                                  [In] string host
                                                  );

            [DllImport(WS2_32, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern IntPtr gethostbyaddr(
                                                  [In] ref int addr,
                                                  [In] int len,
                                                  [In] ProtocolFamily type
                                                  );

            [DllImport(WS2_32, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern int gethostname(
                                                [Out] StringBuilder hostName,
                                                [In] int bufferLength
                                                );

            // this should belong to SafeNativeMethods, but it will not for simplicity
            [DllImport(WS2_32, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern int inet_addr(
                                              [In] string cp
                                              );

            [DllImport(WS2_32, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern int getpeername(
                                                [In] IntPtr socketHandle,
                                                [Out] byte[] socketAddress,
                                                [In, Out] ref int socketAddressSize
                                                );

            [DllImport(WS2_32, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern int getsockopt(
                                               [In] IntPtr socketHandle,
                                               [In] SocketOptionLevel optionLevel,
                                               [In] SocketOptionName optionName,
                                               [Out] out int optionValue,
                                               [In, Out] ref int optionLength
                                               );

            [DllImport(WS2_32, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern int getsockopt(
                                               [In] IntPtr socketHandle,
                                               [In] SocketOptionLevel optionLevel,
                                               [In] SocketOptionName optionName,
                                               [Out] byte[] optionValue,
                                               [In, Out] ref int optionLength
                                               );

            [DllImport(WS2_32, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern int getsockopt(
                                               [In] IntPtr socketHandle,
                                               [In] SocketOptionLevel optionLevel,
                                               [In] SocketOptionName optionName,
                                               [Out] out Linger optionValue,
                                               [In, Out] ref int optionLength
                                               );

            [DllImport(WS2_32, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern int getsockopt(
                                               [In] IntPtr socketHandle,
                                               [In] SocketOptionLevel optionLevel,
                                               [In] SocketOptionName optionName,
                                               [Out] out IPMulticastRequest optionValue,
                                               [In, Out] ref int optionLength
                                               );

            [DllImport(WS2_32, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern int setsockopt(
                                               [In] IntPtr socketHandle,
                                               [In] SocketOptionLevel optionLevel,
                                               [In] SocketOptionName optionName,
                                               [In] ref int optionValue,
                                               [In] int optionLength
                                               );

            [DllImport(WS2_32, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern int setsockopt(
                                               [In] IntPtr socketHandle,
                                               [In] SocketOptionLevel optionLevel,
                                               [In] SocketOptionName optionName,
                                               [In] byte[] optionValue,
                                               [In] int optionLength
                                               );

            [DllImport(WS2_32, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern int setsockopt(
                                               [In] IntPtr socketHandle,
                                               [In] SocketOptionLevel optionLevel,
                                               [In] SocketOptionName optionName,
                                               [In] ref Linger linger,
                                               [In] int optionLength
                                               );

            [DllImport(WS2_32, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern int setsockopt(
                                               [In] IntPtr socketHandle,
                                               [In] SocketOptionLevel optionLevel,
                                               [In] SocketOptionName optionName,
                                               [In] ref IPMulticastRequest mreq,
                                               [In] int optionLength
                                               );

            [DllImport(WS2_32, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern int connect(
                                            [In] IntPtr socketHandle,
                                            [In] byte[] socketAddress,
                                            [In] int socketAddressSize
                                            );

            [DllImport(WS2_32, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern int send(
                                         [In] IntPtr socketHandle,
                                         [In] IntPtr pinnedBuffer,
                                         [In] int len,
                                         [In] SocketFlags socketFlags
                                         );

            [DllImport(WS2_32, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern int recv(
                                         [In] IntPtr socketHandle,
                                         [In] IntPtr pinnedBuffer,
                                         [In] int len,
                                         [In] SocketFlags socketFlags
                                         );

            [DllImport(WS2_32, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern int closesocket(
                                                [In] IntPtr socketHandle
                                                );

            [DllImport(WS2_32, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern IntPtr accept(
                                           [In] IntPtr socketHandle,
                                           [Out] byte[] socketAddress,
                                           [In, Out] ref int socketAddressSize
                                           );

            [DllImport(WS2_32, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern int listen(
                                           [In] IntPtr socketHandle,
                                           [In] int backlog
                                           );

            [DllImport(WS2_32, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern int bind(
                                         [In] IntPtr socketHandle,
                                         [In] byte[] socketAddress,
                                         [In] int socketAddressSize
                                         );

            [DllImport(WS2_32, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern int shutdown(
                                             [In] IntPtr socketHandle,
                                             [In] int how
                                             );

            [DllImport(WS2_32, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern int sendto(
                                           [In] IntPtr socketHandle,
                                           [In] IntPtr pinnedBuffer,
                                           [In] int len,
                                           [In] SocketFlags socketFlags,
                                           [In] byte[] socketAddress,
                                           [In] int socketAddressSize
                                           );

            [DllImport(WS2_32, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern int recvfrom(
                                             [In] IntPtr socketHandle,
                                             [In] IntPtr pinnedBuffer,
                                             [In] int len,
                                             [In] SocketFlags socketFlags,
                                             [Out] byte[] socketAddress,
                                             [In, Out] ref int socketAddressSize
                                             );

            [DllImport(WS2_32, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern int getsockname(
                                                [In] IntPtr socketHandle,
                                                [Out] byte[] socketAddress,
                                                [In, Out] ref int socketAddressSize
                                                );

            [DllImport(WS2_32, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern int select(
                                           [In] int ignoredParameter,
                                           [In, Out] ref FileDescriptorSet readfds,
                                           [In, Out] ref FileDescriptorSet writefds,
                                           [In, Out] ref FileDescriptorSet exceptfds,
                                           [In] ref TimeValue timeout
                                           );

            [DllImport(WS2_32, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern int select(
                                           [In] int ignoredParameter,
                                           [In, Out] ref FileDescriptorSet readfds,
                                           [In, Out] ref FileDescriptorSet writefds,
                                           [In, Out] ref FileDescriptorSet exceptfds,
                                           [In] IntPtr nullTimeout
                                           );

            [DllImport(WS2_32, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern int select(
                                           [In] int ignoredParameter,
                                           [In, Out] ref FileDescriptorSet readfds,
                                           [In] IntPtr ignoredA,
                                           [In] IntPtr ignoredB,
                                           [In] ref TimeValue timeout
                                           );

            [DllImport(WS2_32, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern int select(
                                           [In] int ignoredParameter,
                                           [In, Out] ref FileDescriptorSet readfds,
                                           [In] IntPtr ignoredA,
                                           [In] IntPtr ignoredB,
                                           [In] IntPtr nullTimeout
                                           );

            [DllImport(WS2_32, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern int select(
                                           [In] int ignoredParameter,
                                           [In] IntPtr ignoredA,
                                           [In, Out] ref FileDescriptorSet writefds,
                                           [In] IntPtr ignoredB,
                                           [In] ref TimeValue timeout
                                           );

            [DllImport(WS2_32, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern int select(
                                           [In] int ignoredParameter,
                                           [In] IntPtr ignoredA,
                                           [In, Out] ref FileDescriptorSet writefds,
                                           [In] IntPtr ignoredB,
                                           [In] IntPtr nullTimeout
                                           );


            [DllImport(WS2_32, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern int select(
                                           [In] int ignoredParameter,
                                           [In] IntPtr ignoredA,
                                           [In] IntPtr ignoredB,
                                           [In, Out] ref FileDescriptorSet exceptfds,
                                           [In] ref TimeValue timeout
                                           );

            [DllImport(WS2_32, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern int select(
                                           [In] int ignoredParameter,
                                           [In] IntPtr ignoredA,
                                           [In] IntPtr ignoredB,
                                           [In, Out] ref FileDescriptorSet exceptfds,
                                           [In] IntPtr nullTimeout
                                           );

            [DllImport(WS2_32, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern int WSASend(
                                              [In] IntPtr socketHandle,
                                              [In] ref WSABuffer Buffer,
                                              [In] int BufferCount,
                                              [In] IntPtr bytesTransferred,
                                              [In] SocketFlags socketFlags,
                                              [In] IntPtr overlapped,
                                              [In] IntPtr completionRoutine
                                              );

            [DllImport(WS2_32, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern int WSASend(
                                              [In] IntPtr socketHandle,
                                              [In] WSABuffer[] BufferArray,
                                              [In] int BufferCount,
                                              [In] IntPtr bytesTransferred,
                                              [In] SocketFlags socketFlags,
                                              [In] IntPtr overlapped,
                                              [In] IntPtr completionRoutine
                                              );

            [DllImport(WS2_32, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern int WSASendTo(
                                                [In] IntPtr socketHandle,
                                                [In] ref WSABuffer Buffer,
                                                [In] int BufferCount,
                                                [In] IntPtr BytesWritten,
                                                [In] SocketFlags socketFlags,
                                                [In] byte[] socketAddress,
                                                [In] int socketAddressSize,
                                                [In] IntPtr overlapped,
                                                [In] IntPtr completionRoutine
                                                );

            [DllImport(WS2_32, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern int WSARecv(
                                              [In] IntPtr socketHandle,
                                              [In, Out] ref WSABuffer Buffer,
                                              [In] int BufferCount,
                                              [In] IntPtr bytesTransferred,
                                              [In, Out] ref SocketFlags socketFlags,
                                              [In] IntPtr overlapped,
                                              [In] IntPtr completionRoutine
                                              );

            [DllImport(WS2_32, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern int WSARecvFrom(
                                                  [In] IntPtr socketHandle,
                                                  [In, Out] ref WSABuffer Buffer,
                                                  [In] int BufferCount,
                                                  [In] IntPtr bytesTransferred,
                                                  [In, Out] ref SocketFlags socketFlags,
                                                  [In] IntPtr socketAddressPointer,
                                                  [In] IntPtr socketAddressSizePointer,
                                                  [In] IntPtr overlapped,
                                                  [In] IntPtr completionRoutine
                                                  );


            [DllImport(WS2_32, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern int WSAEventSelect(
                                                     [In] IntPtr socketHandle,
                                                     [In] IntPtr Event,
                                                     [In] AsyncEventBits NetworkEvents
                                                     );

            [DllImport(WS2_32, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern IntPtr WSASocket(
                                                    [In] AddressFamily addressFamily,
                                                    [In] SocketType socketType,
                                                    [In] ProtocolType protocolType,
                                                    [In] IntPtr protocolInfo, // will be WSAProtcolInfo protocolInfo once we include QOS APIs
                                                    [In] uint group,
                                                    [In] SocketConstructorFlags flags
                                                    );

            [DllImport(WS2_32, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern int WSAIoctl(
                                                [In] IntPtr socketHandle,
                                                [In] int ioControlCode,
                                                [In] byte[] inBuffer,
                                                [In] int inBufferSize,
                                                [Out] byte[] outBuffer,
                                                [In] int outBufferSize,
                                                [Out] out int bytesTransferred,
                                                [In] IntPtr overlapped,
                                                [In] IntPtr completionRoutine
                                                );

            [DllImport(WS2_32, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern int WSAEnumNetworkEvents(
                                                     [In] IntPtr socketHandle,
                                                     [In] IntPtr Event,
                                                     [In, Out] ref NetworkEvents networkEvents
                                                     );

/*
            [DllImport(WS2_32, CharSet=CharSet.Ansi, SetLastError=true)]
            internal static extern int WSAStringToAddress(
                [In] string addressString,
                [In] AddressFamily addressFamily,
                [In] IntPtr lpProtocolInfo, // always passing in a 0
                [Out] byte[] socketAddress,
                [In, Out] ref int socketAddressSize );
*/


        }; // class UnsafeNclNativeMethods.OSSOCK


    }; // class UnsafeNclNativeMethods

} // class namespace System.Net
