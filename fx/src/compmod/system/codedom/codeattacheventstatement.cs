//------------------------------------------------------------------------------
// <copyright file="CodeAttachEventStatement.cs" company="Microsoft">
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

    /// <include file='doc\CodeAttachEventStatement.uex' path='docs/doc[@for="CodeAttachEventStatement"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents a event attach statement.
    ///    </para>
    /// </devdoc>
    [
        Serializable,
    ]
    public class CodeAttachEventStatement : CodeStatement {
        private CodeEventReferenceExpression eventRef;
        private CodeExpression listener;

        /// <include file='doc\CodeAttachEventStatement.uex' path='docs/doc[@for="CodeAttachEventStatement.CodeAttachEventStatement"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of <see cref='System.CodeDom.CodeAttachEventStatement'/>.
        ///    </para>
        /// </devdoc>
        public CodeAttachEventStatement() {
        }

        /// <include file='doc\CodeAttachEventStatement.uex' path='docs/doc[@for="CodeAttachEventStatement.CodeAttachEventStatement1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.CodeDom.CodeAttachEventStatement'/> class using the specified arguments.
        ///    </para>
        /// </devdoc>
        public CodeAttachEventStatement(CodeEventReferenceExpression eventRef, CodeExpression listener) {
            this.eventRef = eventRef;
            this.listener = listener;
        }

        /// <include file='doc\CodeAttachEventStatement.uex' path='docs/doc[@for="CodeAttachEventStatement.CodeAttachEventStatement2"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public CodeAttachEventStatement(CodeExpression targetObject, string eventName, CodeExpression listener) {
            this.eventRef = new CodeEventReferenceExpression(targetObject, eventName);
            this.listener = listener;
        }

        /// <include file='doc\CodeAttachEventStatement.uex' path='docs/doc[@for="CodeAttachEventStatement.Event"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The event to attach a listener to.
        ///    </para>
        /// </devdoc>
        public CodeEventReferenceExpression Event {
            get {
                if (eventRef == null) {
                    return new CodeEventReferenceExpression();
                }
                return eventRef;
            }
            set {
                eventRef = value;
            }
        }

        /// <include file='doc\CodeAttachEventStatement.uex' path='docs/doc[@for="CodeAttachEventStatement.Listener"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The new listener.
        ///    </para>
        /// </devdoc>
        public CodeExpression Listener {
            get {
                return listener;
            }
            set {
                listener = value;
            }
        }
    }
}
