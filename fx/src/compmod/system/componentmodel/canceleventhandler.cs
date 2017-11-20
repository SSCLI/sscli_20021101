//------------------------------------------------------------------------------
// <copyright file="CancelEventHandler.cs" company="Microsoft">
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

namespace System.ComponentModel {
    using System.ComponentModel;

    using System.Diagnostics;

    using System;

    /// <include file='doc\CancelEventHandler.uex' path='docs/doc[@for="CancelEventHandler"]/*' />
    /// <devdoc>
    ///    <para>Represents the method that will handle the event raised when canceling an
    ///       event.</para>
    /// </devdoc>
    public delegate void CancelEventHandler(object sender, CancelEventArgs e);
}
