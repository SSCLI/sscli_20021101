//------------------------------------------------------------------------------
// <copyright file="CodeGotoStatement.cs" company="Microsoft">
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

namespace System.CodeDom {

    using System.Diagnostics;
    using System;
    using Microsoft.Win32;
    using System.Collections;
    using System.Runtime.InteropServices;

    /// <include file='doc\CodeGotoStatement.uex' path='docs/doc[@for="CodeGotoStatement"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [
        Serializable,
    ]
    public class CodeGotoStatement : CodeStatement {
        private string label;

        /// <include file='doc\CodeGotoStatement.uex' path='docs/doc[@for="CodeGotoStatement.CodeGotoStatement"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public CodeGotoStatement(string label) {
            Label = label;
        }

        /// <include file='doc\CodeGotoStatement.uex' path='docs/doc[@for="CodeGotoStatement.Label"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string Label {
            get {
                return label;
            }
            set {
                this.label = value;
            }
        }
    }
}
