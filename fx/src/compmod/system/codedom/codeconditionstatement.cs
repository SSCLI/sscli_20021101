//------------------------------------------------------------------------------
// <copyright file="CodeConditionStatement.cs" company="Microsoft">
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

    /// <include file='doc\CodeConditionStatement.uex' path='docs/doc[@for="CodeConditionStatement"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents a basic if statement.
    ///    </para>
    /// </devdoc>
    [
        Serializable,
    ]
    public class CodeConditionStatement : CodeStatement {
        private CodeExpression condition;
        private CodeStatementCollection trueStatments = new CodeStatementCollection();
        private CodeStatementCollection falseStatments = new CodeStatementCollection();

        /// <include file='doc\CodeConditionStatement.uex' path='docs/doc[@for="CodeConditionStatement.CodeConditionStatement"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of <see cref='System.CodeDom.CodeConditionStatement'/>.
        ///    </para>
        /// </devdoc>
        public CodeConditionStatement() {
        }

        /// <include file='doc\CodeConditionStatement.uex' path='docs/doc[@for="CodeConditionStatement.CodeConditionStatement1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of <see cref='System.CodeDom.CodeConditionStatement'/>.
        ///    </para>
        /// </devdoc>
        public CodeConditionStatement(CodeExpression condition, params CodeStatement[] trueStatements) {
            Condition = condition;
            TrueStatements.AddRange(trueStatements);
        }

        /// <include file='doc\CodeConditionStatement.uex' path='docs/doc[@for="CodeConditionStatement.CodeConditionStatement2"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of <see cref='System.CodeDom.CodeConditionStatement'/> that can represent an if..
        ///       else statement.
        ///    </para>
        /// </devdoc>
        public CodeConditionStatement(CodeExpression condition, CodeStatement[] trueStatements, CodeStatement[] falseStatements) {
            Condition = condition;
            TrueStatements.AddRange(trueStatements);
            FalseStatements.AddRange(falseStatements);
        }

        /// <include file='doc\CodeConditionStatement.uex' path='docs/doc[@for="CodeConditionStatement.Condition"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets
        ///       the condition to test for <see langword='true'/>.
        ///    </para>
        /// </devdoc>
        public CodeExpression Condition {
            get {
                return condition;
            }
            set {
                condition = value;
            }
        }

        /// <include file='doc\CodeConditionStatement.uex' path='docs/doc[@for="CodeConditionStatement.TrueStatements"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets
        ///       the statements to execute if test condition is <see langword='true'/>.
        ///    </para>
        /// </devdoc>
        public CodeStatementCollection TrueStatements {
            get {
                return trueStatments;
            }
        }

        /// <include file='doc\CodeConditionStatement.uex' path='docs/doc[@for="CodeConditionStatement.FalseStatements"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets
        ///       the statements to
        ///       execute if test condition is <see langword='false'/> and there is an else
        ///       clause.
        ///    </para>
        /// </devdoc>
        public CodeStatementCollection FalseStatements {
            get {
                return falseStatments;
            }
        }
    }
}
