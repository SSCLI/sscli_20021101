//------------------------------------------------------------------------------
// <copyright file="SocketFlags.cs" company="Microsoft">
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

    /// <include file='doc\SocketFlags.uex' path='docs/doc[@for="SocketFlags"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Provides constant values for socket messages.
    ///    </para>
    /// </devdoc>
    //UEUE
    [Flags]
    public enum SocketFlags {

        /// <include file='doc\SocketFlags.uex' path='docs/doc[@for="SocketFlags.None"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Use no flags for this call.
        ///    </para>
        /// </devdoc>
        None                = 0x0000,

        /// <include file='doc\SocketFlags.uex' path='docs/doc[@for="SocketFlags.OutOfBand"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Process out-of-band data.
        ///    </para>
        /// </devdoc>
        OutOfBand           = 0x0001,

        /// <include file='doc\SocketFlags.uex' path='docs/doc[@for="SocketFlags.Peek"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Peek at incoming message.
        ///    </para>
        /// </devdoc>
        Peek                = 0x0002,

        /// <include file='doc\SocketFlags.uex' path='docs/doc[@for="SocketFlags.DontRoute"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Send without using routing tables.
        ///    </para>
        /// </devdoc>
        DontRoute           = 0x0004,

        /// <include file='doc\SocketFlags.uex' path='docs/doc[@for="SocketFlags.MaxIOVectorLength"]/*' />
        // see: http://as400bks.rochester.ibm.com/pubs/html/as400/v4r5/ic2978/info/apis/recvms.htm
        MaxIOVectorLength   = 0x0010,

        /// <include file='doc\SocketFlags.uex' path='docs/doc[@for="SocketFlags.Partial"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Partial send or recv for message.
        ///    </para>
        /// </devdoc>
        Partial             = 0x8000,

    }; // enum SocketFlags


/*
MSG_DONTROUTE
Specifies that the data should not be subject to routing. A WinSock service 
provider may choose to ignore this flag;.

MSG_OOB
Send out-of-band data (stream style socket such as SOCK_STREAM only).

MSG_PARTIAL
Specifies that lpBuffers only contains a partial message. Note 
that the error code WSAEOPNOTSUPP will be returnedthis flag is ignored by 
transports which do not support partial message transmissions.

MSG_INTERRUPT // not supported (Win16)
Specifies that the function is being called in interrupt context.
The service provider must not make any Windows systems calls. Note that 
this is applicable only to Win16 environments and only for protocols that 
have the XP1_INTERRUPT bit set in the PROTOCOL_INFO struct.
*/  


} // namespace System.Net.Sockets
