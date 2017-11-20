//------------------------------------------------------------------------------
// <copyright file="CodeEventReferenceExpression.cs" company="Microsoft">
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

    /// <include file='doc\CodeEventReferenceExpression.uex' path='docs/doc[@for="CodeEventReferenceExpression"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [
        Serializable,
    ]
    public class CodeEventReferenceExpression : CodeExpression {
        private CodeExpression targetObject;
        private string eventName;

        /// <include file='doc\CodeEventReferenceExpression.uex' path='docs/doc[@for="CodeEventReferenceExpression.CodeEventReferenceExpression"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public CodeEventReferenceExpression() {
        }

        /// <include file='doc\CodeEventReferenceExpression.uex' path='docs/doc[@for="CodeEventReferenceExpression.CodeEventReferenceExpression1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public CodeEventReferenceExpression(CodeExpression targetObject, string eventName) {
            this.targetObject = targetObject;
            this.eventName = eventName;
        }

        /// <include file='doc\CodeEventReferenceExpression.uex' path='docs/doc[@for="CodeEventReferenceExpression.TargetObject"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public CodeExpression TargetObject {
            get {
                return targetObject;
            }
            set {
                this.targetObject = value;
            }
        }

        /// <include file='doc\CodeEventReferenceExpression.uex' path='docs/doc[@for="CodeEventReferenceExpression.EventName"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string EventName {
            get {
                return (eventName == null) ? string.Empty : eventName;
            }
            set {
                eventName = value;
            }
        }
    }
}
