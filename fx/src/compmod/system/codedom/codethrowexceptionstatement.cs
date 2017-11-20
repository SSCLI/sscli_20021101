//------------------------------------------------------------------------------
// <copyright file="CodeThrowExceptionStatement.cs" company="Microsoft">
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

    /// <include file='doc\CodeThrowExceptionStatement.uex' path='docs/doc[@for="CodeThrowExceptionStatement"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents
    ///       a statement that throws an exception.
    ///    </para>
    /// </devdoc>
    [
        Serializable,
    ]
    public class CodeThrowExceptionStatement : CodeStatement {
        private CodeExpression toThrow;

        /// <include file='doc\CodeThrowExceptionStatement.uex' path='docs/doc[@for="CodeThrowExceptionStatement.CodeThrowExceptionStatement"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of <see cref='System.CodeDom.CodeThrowExceptionStatement'/>.
        ///    </para>
        /// </devdoc>
        public CodeThrowExceptionStatement() {
        }
        
        /// <include file='doc\CodeThrowExceptionStatement.uex' path='docs/doc[@for="CodeThrowExceptionStatement.CodeThrowExceptionStatement1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of <see cref='System.CodeDom.CodeThrowExceptionStatement'/> using the specified statement.
        ///    </para>
        /// </devdoc>
        public CodeThrowExceptionStatement(CodeExpression toThrow) {
            ToThrow = toThrow;
        }

        /// <include file='doc\CodeThrowExceptionStatement.uex' path='docs/doc[@for="CodeThrowExceptionStatement.ToThrow"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets
        ///       the expression to throw.
        ///    </para>
        /// </devdoc>
        public CodeExpression ToThrow {
            get {
                return toThrow;
            }
            set {
                toThrow = value;
            }
        }
    }
}
