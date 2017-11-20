//------------------------------------------------------------------------------
// <copyright file="WarningException.cs" company="Microsoft">
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

/*
 */
namespace System.ComponentModel {    
    using System.Diagnostics;
    using System;
    using System.Runtime.InteropServices;
    using Microsoft.Win32;

    /// <include file='doc\WarningException.uex' path='docs/doc[@for="WarningException"]/*' />
    /// <devdoc>
    ///    <para>Specifies an exception that is handled as a warning instead of an error.</para>
    /// </devdoc>
    public class WarningException : SystemException {
        private readonly string helpUrl;
        private readonly string helpTopic;

        /// <include file='doc\WarningException.uex' path='docs/doc[@for="WarningException.WarningException"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.ComponentModel.WarningException'/> class with 
        ///    the specified message and no Help file.</para>
        /// </devdoc>
        public WarningException(string message) : this(message, null, null) {
        }

        /// <include file='doc\WarningException.uex' path='docs/doc[@for="WarningException.WarningException1"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.ComponentModel.WarningException'/> class with 
        ///    the specified message, and with access to the specified Help file.</para>
        /// </devdoc>
        public WarningException(string message, string helpUrl) : this(message, helpUrl, null) {
        }

        /// <include file='doc\WarningException.uex' path='docs/doc[@for="WarningException.WarningException2"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.ComponentModel.WarningException'/> class with the 
        ///    specified message, and with access to the specified Help file and topic.</para>
        /// </devdoc>
        public WarningException(string message, string helpUrl, string helpTopic)
            : base(message) {
            this.helpUrl = helpUrl;
            this.helpTopic = helpTopic;
        }

        /// <include file='doc\WarningException.uex' path='docs/doc[@for="WarningException.HelpUrl"]/*' />
        /// <devdoc>
        ///    <para> Specifies the Help file associated with the 
        ///       warning. This field is read-only.</para>
        /// </devdoc>
        public string HelpUrl {
            get {
                return helpUrl;
            }
        }

        /// <include file='doc\WarningException.uex' path='docs/doc[@for="WarningException.HelpTopic"]/*' />
        /// <devdoc>
        ///    <para> Specifies the 
        ///       Help topic associated with the warning. This field is read-only. </para>
        /// </devdoc>
        public string HelpTopic {
            get {
                return helpTopic;
            }
        }
    }
}
