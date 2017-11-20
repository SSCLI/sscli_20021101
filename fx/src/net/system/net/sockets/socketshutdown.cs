//------------------------------------------------------------------------------
// <copyright file="SocketShutdown.cs" company="Microsoft">
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

    /// <include file='doc\SocketShutdown.uex' path='docs/doc[@for="SocketShutdown"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Defines constants used by the <see cref='System.Net.Sockets.Socket.Shutdown'/> method.
    ///    </para>
    /// </devdoc>
    public enum SocketShutdown {
        /// <include file='doc\SocketShutdown.uex' path='docs/doc[@for="SocketShutdown.Receive"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Shutdown sockets for receive.
        ///    </para>
        /// </devdoc>
        Receive   = 0x00,
        /// <include file='doc\SocketShutdown.uex' path='docs/doc[@for="SocketShutdown.Send"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Shutdown socket for send.
        ///    </para>
        /// </devdoc>
        Send      = 0x01,
        /// <include file='doc\SocketShutdown.uex' path='docs/doc[@for="SocketShutdown.Both"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Shutdown socket for both send and receive.
        ///    </para>
        /// </devdoc>
        Both      = 0x02,

    }; // enum SocketShutdown


} // namespace System.Net.Sockets
