//------------------------------------------------------------------------------
// <copyright file="CodeSnippetStatement.cs" company="Microsoft">
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

    /// <include file='doc\CodeSnippetStatement.uex' path='docs/doc[@for="CodeSnippetStatement"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents a snippet statement.
    ///    </para>
    /// </devdoc>
    [
        Serializable,
    ]
    public class CodeSnippetStatement : CodeStatement {
        private string value;

        /// <include file='doc\CodeSnippetStatement.uex' path='docs/doc[@for="CodeSnippetStatement.CodeSnippetStatement"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of <see cref='System.CodeDom.CodeSnippetStatement'/>.
        ///    </para>
        /// </devdoc>
        public CodeSnippetStatement() {
        }
        
        /// <include file='doc\CodeSnippetStatement.uex' path='docs/doc[@for="CodeSnippetStatement.CodeSnippetStatement1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of <see cref='System.CodeDom.CodeSnippetStatement'/> using the specified snippet
        ///       of code.
        ///    </para>
        /// </devdoc>
        public CodeSnippetStatement(string value) {
            Value = value;
        }

        /// <include file='doc\CodeSnippetStatement.uex' path='docs/doc[@for="CodeSnippetStatement.Value"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the snippet statement.
        ///    </para>
        /// </devdoc>
        public string Value {
            get {
                return (value == null) ? string.Empty : value;
            }
            set {
                this.value = value;
            }
        }
    }
}
