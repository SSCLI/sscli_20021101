//------------------------------------------------------------------------------
// <copyright file="CodeBinaryOperatorExpression.cs" company="Microsoft">
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

    /// <include file='doc\CodeBinaryOperatorExpression.uex' path='docs/doc[@for="CodeBinaryOperatorExpression"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents a binary operator expression.
    ///    </para>
    /// </devdoc>
    [
        Serializable,
    ]
    public class CodeBinaryOperatorExpression : CodeExpression {
        private CodeBinaryOperatorType op;
        private CodeExpression left;
        private CodeExpression right;

        /// <include file='doc\CodeBinaryOperatorExpression.uex' path='docs/doc[@for="CodeBinaryOperatorExpression.CodeBinaryOperatorExpression"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of <see cref='System.CodeDom.CodeBinaryOperatorExpression'/>.
        ///    </para>
        /// </devdoc>
        public CodeBinaryOperatorExpression() {
        }

        /// <include file='doc\CodeBinaryOperatorExpression.uex' path='docs/doc[@for="CodeBinaryOperatorExpression.CodeBinaryOperatorExpression1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of <see cref='System.CodeDom.CodeBinaryOperatorExpression'/>
        ///       using the specified
        ///       parameters.
        ///    </para>
        /// </devdoc>
        public CodeBinaryOperatorExpression(CodeExpression left, CodeBinaryOperatorType op, CodeExpression right) {
            Right = right;
            Operator = op;
            Left = left;
        }

        /// <include file='doc\CodeBinaryOperatorExpression.uex' path='docs/doc[@for="CodeBinaryOperatorExpression.Right"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets
        ///       the code expression on the right of the operator.
        ///    </para>
        /// </devdoc>
        public CodeExpression Right {
            get {
                return right;
            }
            set {
                right = value;
            }
        }

        /// <include file='doc\CodeBinaryOperatorExpression.uex' path='docs/doc[@for="CodeBinaryOperatorExpression.Left"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets
        ///       the code expression on the left of the operator.
        ///    </para>
        /// </devdoc>
        public CodeExpression Left {
            get {
                return left;
            }
            set {
                left = value;
            }
        }

        /// <include file='doc\CodeBinaryOperatorExpression.uex' path='docs/doc[@for="CodeBinaryOperatorExpression.Operator"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets
        ///       the operator in the binary operator expression.
        ///    </para>
        /// </devdoc>
        public CodeBinaryOperatorType Operator {
            get {
                return op;
            }
            set {
                op = value;
            }
        }
    }
}
