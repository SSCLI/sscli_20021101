//------------------------------------------------------------------------------
// <copyright file="CodeIndexerExpression.cs" company="Microsoft">
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

    /// <include file='doc\CodeIndexerExpression.uex' path='docs/doc[@for="CodeIndexerExpression"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents an array indexer expression.
    ///    </para>
    /// </devdoc>
    [
        Serializable,
    ]
    public class CodeIndexerExpression : CodeExpression {
        private CodeExpression targetObject;
        private CodeExpressionCollection indices;

        /// <include file='doc\CodeIndexerExpression.uex' path='docs/doc[@for="CodeIndexerExpression.CodeIndexerExpression"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of <see cref='System.CodeDom.CodeIndexerExpression'/>.
        ///    </para>
        /// </devdoc>
        public CodeIndexerExpression() {
        }

        /// <include file='doc\CodeIndexerExpression.uex' path='docs/doc[@for="CodeIndexerExpression.CodeIndexerExpression1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of <see cref='System.CodeDom.CodeIndexerExpression'/> using the specified target
        ///       object and index.
        ///    </para>
        /// </devdoc>
        public CodeIndexerExpression(CodeExpression targetObject, params CodeExpression[] indices) {
            this.targetObject = targetObject;
            this.indices = new CodeExpressionCollection();
            this.indices.AddRange(indices);
        }

        /// <include file='doc\CodeIndexerExpression.uex' path='docs/doc[@for="CodeIndexerExpression.TargetObject"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets
        ///       the target object.
        ///    </para>
        /// </devdoc>
        public CodeExpression TargetObject {
            get {
                return targetObject;
            }
            set {
                targetObject = value;
            }
        }

        /// <include file='doc\CodeIndexerExpression.uex' path='docs/doc[@for="CodeIndexerExpression.Indices"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets
        ///       the index.
        ///    </para>
        /// </devdoc>
        public CodeExpressionCollection Indices {
            get {
                if (indices == null) {
                    indices = new CodeExpressionCollection();
                }
                return indices;
            }
        }
    }
}
