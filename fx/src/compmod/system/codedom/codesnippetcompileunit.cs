//------------------------------------------------------------------------------
// <copyright file="CodeSnippetCompileUnit.cs" company="Microsoft">
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

    /// <include file='doc\CodeSnippetCompileUnit.uex' path='docs/doc[@for="CodeSnippetCompileUnit"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents a snippet block of code.
    ///    </para>
    /// </devdoc>
    [
        Serializable,
    ]
    public class CodeSnippetCompileUnit : CodeCompileUnit {
        private string value;
        private CodeLinePragma linePragma;

        /// <include file='doc\CodeSnippetCompileUnit.uex' path='docs/doc[@for="CodeSnippetCompileUnit.CodeSnippetCompileUnit"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of <see cref='System.CodeDom.CodeSnippetCompileUnit'/>.
        ///    </para>
        /// </devdoc>
        public CodeSnippetCompileUnit(string value) {
            Value = value;
        }

        /// <include file='doc\CodeSnippetCompileUnit.uex' path='docs/doc[@for="CodeSnippetCompileUnit.Value"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets
        ///       the snippet
        ///       text of the code block to represent.
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

        /// <include file='doc\CodeSnippetCompileUnit.uex' path='docs/doc[@for="CodeSnippetCompileUnit.LinePragma"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The line the code block starts on.
        ///    </para>
        /// </devdoc>
        public CodeLinePragma LinePragma {
            get {
                return linePragma;
            }
            set {
                linePragma = value;
            }
        }
    }
}
