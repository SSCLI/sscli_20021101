//------------------------------------------------------------------------------
// <copyright file="SocketOptionLevel.cs" company="Microsoft">
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

    //
    // Option flags per-socket.
    //

    /// <include file='doc\SocketOptionLevel.uex' path='docs/doc[@for="SocketOptionLevel"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Defines socket option levels for the <see cref='System.Net.Sockets.Socket'/> class.
    ///    </para>
    /// </devdoc>
    //UEUE
    public enum SocketOptionLevel {

        /// <include file='doc\SocketOptionLevel.uex' path='docs/doc[@for="SocketOptionLevel.Socket"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Indicates socket options apply to the socket itself.
        ///    </para>
        /// </devdoc>
        Socket = 0xffff,

        /// <include file='doc\SocketOptionLevel.uex' path='docs/doc[@for="SocketOptionLevel.IP"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Indicates socket options apply to IP sockets.
        ///    </para>
        /// </devdoc>
        IP = ProtocolType.IP,

        /// <include file='doc\SocketOptionLevel.uex' path='docs/doc[@for="SocketOptionLevel.Tcp"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Indicates socket options apply to Tcp sockets.
        ///    </para>
        /// </devdoc>
        Tcp = ProtocolType.Tcp,

        /// <include file='doc\SocketOptionLevel.uex' path='docs/doc[@for="SocketOptionLevel.Udp"]/*' />
        /// <devdoc>
        /// <para>
        /// Indicates socket options apply to Udp sockets.
        /// </para>
        /// </devdoc>
        //UEUE
        Udp = ProtocolType.Udp,

    }; // enum SocketOptionLevel


} // namespace System.Net.Sockets
