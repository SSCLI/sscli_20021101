//------------------------------------------------------------------------------
// <copyright file="CodeExpressionStatement.cs" company="Microsoft">
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

    /// <include file='doc\CodeExpressionStatement.uex' path='docs/doc[@for="CodeExpressionStatement"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents
    ///       a statement that is an expression.
    ///    </para>
    /// </devdoc>
    [
        Serializable,
    ]
    public class CodeExpressionStatement : CodeStatement {
        private CodeExpression expression;

        /// <include file='doc\CodeExpressionStatement.uex' path='docs/doc[@for="CodeExpressionStatement.CodeExpressionStatement"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public CodeExpressionStatement() {
        }
        
        /// <include file='doc\CodeExpressionStatement.uex' path='docs/doc[@for="CodeExpressionStatement.CodeExpressionStatement1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public CodeExpressionStatement(CodeExpression expression) {
            this.expression = expression;
        }

        /// <include file='doc\CodeExpressionStatement.uex' path='docs/doc[@for="CodeExpressionStatement.Expression"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public CodeExpression Expression {
            get {
                return expression;
            }
            set {
                expression = value;
            }
        }
    }
}
