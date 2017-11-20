//------------------------------------------------------------------------------
// <copyright file="CodeRemoveEventStatement.cs" company="Microsoft">
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

    /// <include file='doc\CodeRemoveEventStatement.uex' path='docs/doc[@for="CodeRemoveEventStatement"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents a event detach statement.
    ///    </para>
    /// </devdoc>
    [
        Serializable,
    ]
    public class CodeRemoveEventStatement : CodeStatement {
        private CodeEventReferenceExpression eventRef;
        private CodeExpression listener;

        /// <include file='doc\CodeRemoveEventStatement.uex' path='docs/doc[@for="CodeRemoveEventStatement.CodeRemoveEventStatement"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of <see cref='System.CodeDom.CodeRemoveEventStatement'/>.
        ///    </para>
        /// </devdoc>
        public CodeRemoveEventStatement() {
        }

        /// <include file='doc\CodeRemoveEventStatement.uex' path='docs/doc[@for="CodeRemoveEventStatement.CodeRemoveEventStatement1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.CodeDom.CodeRemoveEventStatement'/> class using the specified arguments.
        ///    </para>
        /// </devdoc>
        public CodeRemoveEventStatement(CodeEventReferenceExpression eventRef, CodeExpression listener) {
            this.eventRef = eventRef;
            this.listener = listener;
        }

        /// <include file='doc\CodeRemoveEventStatement.uex' path='docs/doc[@for="CodeRemoveEventStatement.CodeRemoveEventStatement2"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public CodeRemoveEventStatement(CodeExpression targetObject, string eventName, CodeExpression listener) {
            this.eventRef = new CodeEventReferenceExpression(targetObject, eventName);
            this.listener = listener;
        }

        /// <include file='doc\CodeRemoveEventStatement.uex' path='docs/doc[@for="CodeRemoveEventStatement.Event"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public CodeEventReferenceExpression Event{
            get {
                if (eventRef == null) {
                    eventRef = new CodeEventReferenceExpression();
                }
                return eventRef;
            }
            set {
                eventRef = value;
            }
        }

       /// <include file='doc\CodeRemoveEventStatement.uex' path='docs/doc[@for="CodeRemoveEventStatement.Listener"]/*' />
       /// <devdoc>
        ///    <para>
        ///       The listener.
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
