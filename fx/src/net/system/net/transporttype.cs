//------------------------------------------------------------------------------
// <copyright file="TransportType.cs" company="Microsoft">
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
    using System;


    /// <include file='doc\TransportType.uex' path='docs/doc[@for="TransportType"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Defines the transport type allowed for the socket.
    ///    </para>
    /// </devdoc>
    public  enum TransportType {
        /// <include file='doc\TransportType.uex' path='docs/doc[@for="TransportType.Udp"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Udp connections are allowed.
        ///    </para>
        /// </devdoc>
        Udp     = 0x1,
        /// <include file='doc\TransportType.uex' path='docs/doc[@for="TransportType.Connectionless"]/*' />
        Connectionless = 1,
        /// <include file='doc\TransportType.uex' path='docs/doc[@for="TransportType.Tcp"]/*' />
        /// <devdoc>
        ///    <para>
        ///       TCP connections are allowed.
        ///    </para>
        /// </devdoc>
        Tcp     = 0x2,
        /// <include file='doc\TransportType.uex' path='docs/doc[@for="TransportType.ConnectionOriented"]/*' />
        ConnectionOriented = 2,
        /// <include file='doc\TransportType.uex' path='docs/doc[@for="TransportType.All"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Any connection is allowed.
        ///    </para>
        /// </devdoc>
        All     = 0x3

    } // enum TransportType

} // namespace System.Net
