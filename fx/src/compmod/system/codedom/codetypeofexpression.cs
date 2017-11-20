//------------------------------------------------------------------------------
// <copyright file="CodeTypeOfExpression.cs" company="Microsoft">
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

    /// <include file='doc\CodeTypeOfExpression.uex' path='docs/doc[@for="CodeTypeOfExpression"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents a TypeOf expression.
    ///    </para>
    /// </devdoc>
    [
        Serializable,
    ]
    public class CodeTypeOfExpression : CodeExpression {
        private CodeTypeReference type;

        /// <include file='doc\CodeTypeOfExpression.uex' path='docs/doc[@for="CodeTypeOfExpression.CodeTypeOfExpression"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of <see cref='System.CodeDom.CodeTypeOfExpression'/>.
        ///    </para>
        /// </devdoc>
        public CodeTypeOfExpression() {
        }

        /// <include file='doc\CodeTypeOfExpression.uex' path='docs/doc[@for="CodeTypeOfExpression.CodeTypeOfExpression1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of <see cref='System.CodeDom.CodeTypeOfExpression'/>.
        ///    </para>
        /// </devdoc>
        public CodeTypeOfExpression(CodeTypeReference type) {
            Type = type;
        }

        /// <include file='doc\CodeTypeOfExpression.uex' path='docs/doc[@for="CodeTypeOfExpression.CodeTypeOfExpression2"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public CodeTypeOfExpression(string type) {
            Type = new CodeTypeReference(type);
        }

        /// <include file='doc\CodeTypeOfExpression.uex' path='docs/doc[@for="CodeTypeOfExpression.CodeTypeOfExpression3"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public CodeTypeOfExpression(Type type) {
            Type = new CodeTypeReference(type);
        }

        /// <include file='doc\CodeTypeOfExpression.uex' path='docs/doc[@for="CodeTypeOfExpression.Type"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the data type.
        ///    </para>
        /// </devdoc>
        public CodeTypeReference Type {
            get {
                if (type == null) {
                    type = new CodeTypeReference("");
                }
                return type;
            }
            set {
                type = value;
            }
        }
    }
}
