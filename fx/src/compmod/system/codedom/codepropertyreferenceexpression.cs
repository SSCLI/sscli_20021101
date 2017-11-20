//------------------------------------------------------------------------------
// <copyright file="CodePropertyReferenceExpression.cs" company="Microsoft">
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

    /// <include file='doc\CodePropertyReferenceExpression.uex' path='docs/doc[@for="CodePropertyReferenceExpression"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents a reference to a property.
    ///    </para>
    /// </devdoc>
    [
        Serializable,
    ]
    public class CodePropertyReferenceExpression : CodeExpression {
        private CodeExpression targetObject;
        private string propertyName;
        private CodeExpressionCollection parameters = new CodeExpressionCollection();

        /// <include file='doc\CodePropertyReferenceExpression.uex' path='docs/doc[@for="CodePropertyReferenceExpression.CodePropertyReferenceExpression"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of <see cref='System.CodeDom.CodePropertyReferenceExpression'/>.
        ///    </para>
        /// </devdoc>
        public CodePropertyReferenceExpression() {
        }

        /// <include file='doc\CodePropertyReferenceExpression.uex' path='docs/doc[@for="CodePropertyReferenceExpression.CodePropertyReferenceExpression1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of <see cref='System.CodeDom.CodePropertyReferenceExpression'/> using the specified target object and property
        ///       name.
        ///    </para>
        /// </devdoc>
        public CodePropertyReferenceExpression(CodeExpression targetObject, string propertyName) {
            TargetObject = targetObject;
            PropertyName = propertyName;
        }

        /// <include file='doc\CodePropertyReferenceExpression.uex' path='docs/doc[@for="CodePropertyReferenceExpression.TargetObject"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The target object containing the property this <see cref='System.CodeDom.CodePropertyReferenceExpression'/> references.
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

        /// <include file='doc\CodePropertyReferenceExpression.uex' path='docs/doc[@for="CodePropertyReferenceExpression.PropertyName"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The name of the property to reference.
        ///    </para>
        /// </devdoc>
        public string PropertyName {
            get {
                return (propertyName == null) ? string.Empty : propertyName;
            }
            set {
                propertyName = value;
            }
        }
    }
}
