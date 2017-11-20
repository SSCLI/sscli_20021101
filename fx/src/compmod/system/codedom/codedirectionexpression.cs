//------------------------------------------------------------------------------
// <copyright file="CodeDirectionExpression.cs" company="Microsoft">
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

    /// <include file='doc\CodeDirectionExpression.uex' path='docs/doc[@for="CodeDirectionExpression"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [
        Serializable,
    ]
    public class CodeDirectionExpression : CodeExpression {
        private CodeExpression expression;
        private FieldDirection direction = FieldDirection.In;


        /// <include file='doc\CodeDirectionExpression.uex' path='docs/doc[@for="CodeDirectionExpression.CodeDirectionExpression"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public CodeDirectionExpression() {
        }

        /// <include file='doc\CodeDirectionExpression.uex' path='docs/doc[@for="CodeDirectionExpression.CodeDirectionExpression1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public CodeDirectionExpression(FieldDirection direction, CodeExpression expression) {
            this.expression = expression;
            this.direction = direction;
        }

        /// <include file='doc\CodeDirectionExpression.uex' path='docs/doc[@for="CodeDirectionExpression.Expression"]/*' />
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

        /// <include file='doc\CodeDirectionExpression.uex' path='docs/doc[@for="CodeDirectionExpression.Direction"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public FieldDirection Direction {
            get {
                return direction;
            }
            set {
                direction = value;
            }
        }
    }
}
