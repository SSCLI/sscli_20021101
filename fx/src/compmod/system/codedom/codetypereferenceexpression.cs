//------------------------------------------------------------------------------
// <copyright file="CodeTypeReferenceExpression.cs" company="Microsoft">
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

    /// <include file='doc\CodeTypeReferenceExpression.uex' path='docs/doc[@for="CodeTypeReferenceExpression"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents a reference to a type.
    ///    </para>
    /// </devdoc>
    [
        Serializable,
    ]
    public class CodeTypeReferenceExpression : CodeExpression {
        private CodeTypeReference type;

        /// <include file='doc\CodeTypeReferenceExpression.uex' path='docs/doc[@for="CodeTypeReferenceExpression.CodeTypeReferenceExpression"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of <see cref='System.CodeDom.CodeTypeReferenceExpression'/>.
        ///    </para>
        /// </devdoc>
        public CodeTypeReferenceExpression() {
        }

        /// <include file='doc\CodeTypeReferenceExpression.uex' path='docs/doc[@for="CodeTypeReferenceExpression.CodeTypeReferenceExpression1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of <see cref='System.CodeDom.CodeTypeReferenceExpression'/> using the specified type.
        ///    </para>
        /// </devdoc>
        public CodeTypeReferenceExpression(CodeTypeReference type) {
            Type = type;
        }

        /// <include file='doc\CodeTypeReferenceExpression.uex' path='docs/doc[@for="CodeTypeReferenceExpression.CodeTypeReferenceExpression2"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public CodeTypeReferenceExpression(string type) {
            Type = new CodeTypeReference(type);
        }

        /// <include file='doc\CodeTypeReferenceExpression.uex' path='docs/doc[@for="CodeTypeReferenceExpression.CodeTypeReferenceExpression3"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public CodeTypeReferenceExpression(Type type) {
            Type = new CodeTypeReference(type);
        }

        /// <include file='doc\CodeTypeReferenceExpression.uex' path='docs/doc[@for="CodeTypeReferenceExpression.Type"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the type to reference.
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
