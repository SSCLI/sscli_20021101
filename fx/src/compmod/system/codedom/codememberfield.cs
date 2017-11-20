//------------------------------------------------------------------------------
// <copyright file="CodeMemberField.cs" company="Microsoft">
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

    /// <include file='doc\CodeMemberField.uex' path='docs/doc[@for="CodeMemberField"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents a class field member.
    ///    </para>
    /// </devdoc>
    [
        Serializable,
    ]
    public class CodeMemberField : CodeTypeMember {
        private CodeTypeReference type;
        private CodeExpression initExpression;

        /// <include file='doc\CodeMemberField.uex' path='docs/doc[@for="CodeMemberField.CodeMemberField"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new <see cref='System.CodeDom.CodeMemberField'/>.
        ///    </para>
        /// </devdoc>
        public CodeMemberField() {
        }

        /// <include file='doc\CodeMemberField.uex' path='docs/doc[@for="CodeMemberField.CodeMemberField1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new <see cref='System.CodeDom.CodeMemberField'/> with the specified member field type and
        ///       name.
        ///    </para>
        /// </devdoc>
        public CodeMemberField(CodeTypeReference type, string name) {
            Type = type;
            Name = name;
        }

        /// <include file='doc\CodeMemberField.uex' path='docs/doc[@for="CodeMemberField.CodeMemberField2"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public CodeMemberField(string type, string name) {
            Type = new CodeTypeReference(type);
            Name = name;
        }

        /// <include file='doc\CodeMemberField.uex' path='docs/doc[@for="CodeMemberField.CodeMemberField3"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public CodeMemberField(Type type, string name) {
            Type = new CodeTypeReference(type);
            Name = name;
        }

        /// <include file='doc\CodeMemberField.uex' path='docs/doc[@for="CodeMemberField.Type"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the member field type.
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

        /// <include file='doc\CodeMemberField.uex' path='docs/doc[@for="CodeMemberField.InitExpression"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the initialization expression for the member field.
        ///    </para>
        /// </devdoc>
        public CodeExpression InitExpression {
            get {
                return initExpression;
            }
            set {
                initExpression = value;
            }
        }
    }
}
