//------------------------------------------------------------------------------
// <copyright file="CodeLabeledStatement.cs" company="Microsoft">
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

    /// <include file='doc\CodeLabeledStatement.uex' path='docs/doc[@for="CodeLabeledStatement"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [
        Serializable,
    ]
    public class CodeLabeledStatement : CodeStatement {
        private string label;
        private CodeStatement statement;

        /// <include file='doc\CodeLabeledStatement.uex' path='docs/doc[@for="CodeLabeledStatement.CodeLabeledStatement"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public CodeLabeledStatement() {
        }

        /// <include file='doc\CodeLabeledStatement.uex' path='docs/doc[@for="CodeLabeledStatement.CodeLabeledStatement1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public CodeLabeledStatement(string label) {
            this.label = label;
        }

        /// <include file='doc\CodeLabeledStatement.uex' path='docs/doc[@for="CodeLabeledStatement.CodeLabeledStatement2"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public CodeLabeledStatement(string label, CodeStatement statement) {
            this.label = label;
            this.statement = statement;
        }

        /// <include file='doc\CodeLabeledStatement.uex' path='docs/doc[@for="CodeLabeledStatement.Label"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string Label {
            get {
                return (label == null) ? string.Empty : label;
            }
            set {
                this.label = value;
            }
        }

        /// <include file='doc\CodeLabeledStatement.uex' path='docs/doc[@for="CodeLabeledStatement.Statement"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public CodeStatement Statement {
            get {
                return statement;
            }
            set {
                this.statement = value;
            }
        }
    }
}
