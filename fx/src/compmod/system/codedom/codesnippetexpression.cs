//------------------------------------------------------------------------------
// <copyright file="CodeSnippetExpression.cs" company="Microsoft">
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

    /// <include file='doc\CodeSnippetExpression.uex' path='docs/doc[@for="CodeSnippetExpression"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents a snippet expression.
    ///    </para>
    /// </devdoc>
    [
        Serializable,
    ]
    public class CodeSnippetExpression : CodeExpression {
        private string value;

        /// <include file='doc\CodeSnippetExpression.uex' path='docs/doc[@for="CodeSnippetExpression.CodeSnippetExpression"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of <see cref='System.CodeDom.CodeSnippetExpression'/>.
        ///    </para>
        /// </devdoc>
        public CodeSnippetExpression() {
        }
        
        /// <include file='doc\CodeSnippetExpression.uex' path='docs/doc[@for="CodeSnippetExpression.CodeSnippetExpression1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of <see cref='System.CodeDom.CodeSnippetExpression'/> using the specified snippet
        ///       expression.
        ///    </para>
        /// </devdoc>
        public CodeSnippetExpression(string value) {
            Value = value;
        }

        /// <include file='doc\CodeSnippetExpression.uex' path='docs/doc[@for="CodeSnippetExpression.Value"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the snippet expression.
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
