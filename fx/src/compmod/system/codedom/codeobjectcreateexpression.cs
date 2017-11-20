//------------------------------------------------------------------------------
// <copyright file="CodeObjectCreateExpression.cs" company="Microsoft">
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

    /// <include file='doc\CodeObjectCreateExpression.uex' path='docs/doc[@for="CodeObjectCreateExpression"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents an object create expression.
    ///    </para>
    /// </devdoc>
    [
        Serializable,
    ]
    public class CodeObjectCreateExpression : CodeExpression {
        private CodeTypeReference createType;
        private CodeExpressionCollection parameters = new CodeExpressionCollection();

        /// <include file='doc\CodeObjectCreateExpression.uex' path='docs/doc[@for="CodeObjectCreateExpression.CodeObjectCreateExpression"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new <see cref='System.CodeDom.CodeObjectCreateExpression'/>.
        ///    </para>
        /// </devdoc>
        public CodeObjectCreateExpression() {
        }

        /// <include file='doc\CodeObjectCreateExpression.uex' path='docs/doc[@for="CodeObjectCreateExpression.CodeObjectCreateExpression1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new <see cref='System.CodeDom.CodeObjectCreateExpression'/> using the specified type and
        ///       parameters.
        ///    </para>
        /// </devdoc>
        public CodeObjectCreateExpression(CodeTypeReference createType, params CodeExpression[] parameters) {
            CreateType = createType;
            Parameters.AddRange(parameters);
        }

        /// <include file='doc\CodeObjectCreateExpression.uex' path='docs/doc[@for="CodeObjectCreateExpression.CodeObjectCreateExpression2"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public CodeObjectCreateExpression(string createType, params CodeExpression[] parameters) {
            CreateType = new CodeTypeReference(createType);
            Parameters.AddRange(parameters);
        }

        /// <include file='doc\CodeObjectCreateExpression.uex' path='docs/doc[@for="CodeObjectCreateExpression.CodeObjectCreateExpression3"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public CodeObjectCreateExpression(Type createType, params CodeExpression[] parameters) {
            CreateType = new CodeTypeReference(createType);
            Parameters.AddRange(parameters);
        }

        /// <include file='doc\CodeObjectCreateExpression.uex' path='docs/doc[@for="CodeObjectCreateExpression.CreateType"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The type of the object to create.
        ///    </para>
        /// </devdoc>
        public CodeTypeReference CreateType {
            get {
                if (createType == null) {
                    createType = new CodeTypeReference("");
                }
                return createType;
            }
            set {
                createType = value;
            }
        }

        /// <include file='doc\CodeObjectCreateExpression.uex' path='docs/doc[@for="CodeObjectCreateExpression.Parameters"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the parameters to use in creating the
        ///       object.
        ///    </para>
        /// </devdoc>
        public CodeExpressionCollection Parameters {
            get {
                return parameters;
            }
        }
    }
}
