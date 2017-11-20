//------------------------------------------------------------------------------
// <copyright file="CodeAssignStatement.cs" company="Microsoft">
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

    /// <include file='doc\CodeAssignStatement.uex' path='docs/doc[@for="CodeAssignStatement"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents a simple assignment statement.
    ///    </para>
    /// </devdoc>
    [
        Serializable,
    ]
    public class CodeAssignStatement : CodeStatement {
        private CodeExpression left;
        private CodeExpression right;

        /// <include file='doc\CodeAssignStatement.uex' path='docs/doc[@for="CodeAssignStatement.CodeAssignStatement"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of <see cref='System.CodeDom.CodeAssignStatement'/>.
        ///    </para>
        /// </devdoc>
        public CodeAssignStatement() {
        }

        /// <include file='doc\CodeAssignStatement.uex' path='docs/doc[@for="CodeAssignStatement.CodeAssignStatement1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of <see cref='System.CodeDom.CodeAssignStatement'/> that represents the
        ///       specified assignment values.
        ///    </para>
        /// </devdoc>
        public CodeAssignStatement(CodeExpression left, CodeExpression right) {
            Left = left;
            Right = right;
        }

        /// <include file='doc\CodeAssignStatement.uex' path='docs/doc[@for="CodeAssignStatement.Left"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets
        ///       the variable to be assigned to.
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

        /// <include file='doc\CodeAssignStatement.uex' path='docs/doc[@for="CodeAssignStatement.Right"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets
        ///       the value to assign.
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
    }
}
