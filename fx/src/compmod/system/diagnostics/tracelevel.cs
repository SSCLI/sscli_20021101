//------------------------------------------------------------------------------
// <copyright file="TraceLevel.cs" company="Microsoft">
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

namespace System.Diagnostics {
    using System.Diagnostics;

    using System;

    /// <include file='doc\TraceLevel.uex' path='docs/doc[@for="TraceLevel"]/*' />
    /// <devdoc>
    ///    <para>Specifies what messages to output for debugging
    ///       and tracing.</para>
    /// </devdoc>
    public enum TraceLevel {
        /// <include file='doc\TraceLevel.uex' path='docs/doc[@for="TraceLevel.Off"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Output no tracing and debugging
        ///       messages.
        ///    </para>
        /// </devdoc>
        Off     = 0,
        /// <include file='doc\TraceLevel.uex' path='docs/doc[@for="TraceLevel.Error"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Output error-handling messages.
        ///    </para>
        /// </devdoc>
        Error   = 1,
        /// <include file='doc\TraceLevel.uex' path='docs/doc[@for="TraceLevel.Warning"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Output warnings and error-handling
        ///       messages.
        ///    </para>
        /// </devdoc>
        Warning = 2,
        /// <include file='doc\TraceLevel.uex' path='docs/doc[@for="TraceLevel.Info"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Output informational messages, warnings, and error-handling messages.
        ///    </para>
        /// </devdoc>
        Info    = 3,
        /// <include file='doc\TraceLevel.uex' path='docs/doc[@for="TraceLevel.Verbose"]/*' />
        /// <devdoc>
        ///    Output all debugging and tracing messages.
        /// </devdoc>
        Verbose = 4,
    }

}
