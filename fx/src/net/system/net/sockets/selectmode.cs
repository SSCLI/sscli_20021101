//------------------------------------------------------------------------------
// <copyright file="SelectMode.cs" company="Microsoft">
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

    /// <include file='doc\SelectMode.uex' path='docs/doc[@for="SelectMode"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Specifies the mode for polling the status of a socket.
    ///    </para>
    /// </devdoc>
    public enum SelectMode {
        /// <include file='doc\SelectMode.uex' path='docs/doc[@for="SelectMode.SelectRead"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Poll the read status of a socket.
        ///    </para>
        /// </devdoc>
        SelectRead     = 0,
        /// <include file='doc\SelectMode.uex' path='docs/doc[@for="SelectMode.SelectWrite"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Poll the write status of a socket.
        ///    </para>
        /// </devdoc>
        SelectWrite    = 1,
        /// <include file='doc\SelectMode.uex' path='docs/doc[@for="SelectMode.SelectError"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Poll the error status of a socket.
        ///    </para>
        /// </devdoc>
        SelectError    = 2
    } // enum SelectMode
} // namespace System.Net.Sockets
