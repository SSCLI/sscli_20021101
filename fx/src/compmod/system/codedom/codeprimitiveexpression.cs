//------------------------------------------------------------------------------
// <copyright file="CodePrimitiveExpression.cs" company="Microsoft">
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

    /// <include file='doc\CodePrimitiveExpression.uex' path='docs/doc[@for="CodePrimitiveExpression"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents a primitive value.
    ///    </para>
    /// </devdoc>
    [
        Serializable,
    ]
    public class CodePrimitiveExpression : CodeExpression {
        private object value;

        /// <include file='doc\CodePrimitiveExpression.uex' path='docs/doc[@for="CodePrimitiveExpression.CodePrimitiveExpression"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of <see cref='System.CodeDom.CodePrimitiveExpression'/>.
        ///    </para>
        /// </devdoc>
        public CodePrimitiveExpression() {
        }

        /// <include file='doc\CodePrimitiveExpression.uex' path='docs/doc[@for="CodePrimitiveExpression.CodePrimitiveExpression1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of <see cref='System.CodeDom.CodePrimitiveExpression'/> using the specified
        ///       object.
        ///    </para>
        /// </devdoc>
        public CodePrimitiveExpression(object value) {
            Value = value;
        }

        /// <include file='doc\CodePrimitiveExpression.uex' path='docs/doc[@for="CodePrimitiveExpression.Value"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the object to represent.
        ///    </para>
        /// </devdoc>
        public object Value {
            get {
                return value;
            }
            set {
                this.value = value;
            }
        }
    }
}
