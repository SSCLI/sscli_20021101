//------------------------------------------------------------------------------
// <copyright file="CodeIterationStatement.cs" company="Microsoft">
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

    /// <include file='doc\CodeIterationStatement.uex' path='docs/doc[@for="CodeIterationStatement"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents a simple for loop.
    ///    </para>
    /// </devdoc>
    [
        Serializable,
    ]
    public class CodeIterationStatement : CodeStatement {
        private CodeStatement initStatement;
        private CodeExpression testExpression;
        private CodeStatement incrementStatement;
        private CodeStatementCollection statements = new CodeStatementCollection();

        /// <include file='doc\CodeIterationStatement.uex' path='docs/doc[@for="CodeIterationStatement.CodeIterationStatement"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of <see cref='System.CodeDom.CodeIterationStatement'/>.
        ///    </para>
        /// </devdoc>
        public CodeIterationStatement() {
        }

        /// <include file='doc\CodeIterationStatement.uex' path='docs/doc[@for="CodeIterationStatement.CodeIterationStatement1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of <see cref='System.CodeDom.CodeIterationStatement'/>.
        ///    </para>
        /// </devdoc>
        public CodeIterationStatement(CodeStatement initStatement, CodeExpression testExpression, CodeStatement incrementStatement, params CodeStatement[] statements) {
            InitStatement = initStatement;
            TestExpression = testExpression;
            IncrementStatement = incrementStatement;
            Statements.AddRange(statements);
        }

        /// <include file='doc\CodeIterationStatement.uex' path='docs/doc[@for="CodeIterationStatement.InitStatement"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets
        ///       the loop initialization statement.
        ///    </para>
        /// </devdoc>
        public CodeStatement InitStatement {
            get {
                return initStatement;
            }
            set {
                initStatement = value;
            }
        }

        /// <include file='doc\CodeIterationStatement.uex' path='docs/doc[@for="CodeIterationStatement.TestExpression"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets
        ///       the expression to test for.
        ///    </para>
        /// </devdoc>
        public CodeExpression TestExpression {
            get {
                return testExpression;
            }
            set {
                testExpression = value;
            }
        }

        /// <include file='doc\CodeIterationStatement.uex' path='docs/doc[@for="CodeIterationStatement.IncrementStatement"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets
        ///       the per loop cycle increment statement.
        ///    </para>
        /// </devdoc>
        public CodeStatement IncrementStatement {
            get {
                return incrementStatement;
            }
            set {
                incrementStatement = value;
            }
        }

        /// <include file='doc\CodeIterationStatement.uex' path='docs/doc[@for="CodeIterationStatement.Statements"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets
        ///       the statements to be executed within the loop.
        ///    </para>
        /// </devdoc>
        public CodeStatementCollection Statements {
            get {
                return statements;
            }
        }        
    }
}
