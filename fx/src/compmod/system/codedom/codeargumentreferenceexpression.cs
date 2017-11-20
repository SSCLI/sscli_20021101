//------------------------------------------------------------------------------
// <copyright file="CodeArgumentReferenceExpression.cs" company="Microsoft">
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

    /// <include file='doc\CodeArgumentReferenceExpression.uex' path='docs/doc[@for="CodeArgumentReferenceExpression"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [
        Serializable,
    ]
    public class CodeArgumentReferenceExpression : CodeExpression {
        private string parameterName;

        /// <include file='doc\CodeArgumentReferenceExpression.uex' path='docs/doc[@for="CodeArgumentReferenceExpression.CodeArgumentReferenceExpression"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public CodeArgumentReferenceExpression() {
        }

        /// <include file='doc\CodeArgumentReferenceExpression.uex' path='docs/doc[@for="CodeArgumentReferenceExpression.CodeArgumentReferenceExpression1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public CodeArgumentReferenceExpression(string parameterName) {
            this.parameterName = parameterName;
        }


        /// <include file='doc\CodeArgumentReferenceExpression.uex' path='docs/doc[@for="CodeArgumentReferenceExpression.ParameterName"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string ParameterName {
            get {
                return (parameterName == null) ? string.Empty : parameterName;
            }
            set {
                parameterName = value;
            }
        }
    }
}
