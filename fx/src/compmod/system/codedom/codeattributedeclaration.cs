//------------------------------------------------------------------------------
// <copyright file="CodeAttributeDeclaration.cs" company="Microsoft">
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

    /// <include file='doc\CodeAttributeDeclaration.uex' path='docs/doc[@for="CodeAttributeDeclaration"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents a single custom attribute.
    ///    </para>
    /// </devdoc>
    [
        Serializable,
    ]
    public class CodeAttributeDeclaration {
        private string name;
        private CodeAttributeArgumentCollection arguments = new CodeAttributeArgumentCollection();

        /// <include file='doc\CodeAttributeDeclaration.uex' path='docs/doc[@for="CodeAttributeDeclaration.CodeAttributeDeclaration"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of <see cref='System.CodeDom.CodeAttributeDeclaration'/>.
        ///    </para>
        /// </devdoc>
        public CodeAttributeDeclaration() {
        }

        /// <include file='doc\CodeAttributeDeclaration.uex' path='docs/doc[@for="CodeAttributeDeclaration.CodeAttributeDeclaration1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of <see cref='System.CodeDom.CodeAttributeDeclaration'/> using the specified name.
        ///    </para>
        /// </devdoc>
        public CodeAttributeDeclaration(string name) {
            Name = name;
        }

        /// <include file='doc\CodeAttributeDeclaration.uex' path='docs/doc[@for="CodeAttributeDeclaration.CodeAttributeDeclaration2"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of <see cref='System.CodeDom.CodeAttributeDeclaration'/> using the specified
        ///       arguments.
        ///    </para>
        /// </devdoc>
        public CodeAttributeDeclaration(string name, params CodeAttributeArgument[] arguments) {
            Name = name;
            Arguments.AddRange(arguments);
        }

        /// <include file='doc\CodeAttributeDeclaration.uex' path='docs/doc[@for="CodeAttributeDeclaration.Name"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The name of the attribute being declared.
        ///    </para>
        /// </devdoc>
        public string Name {
            get {
                return (name == null) ? string.Empty : name;
            }
            set {
                name = value;
            }
        }

        /// <include file='doc\CodeAttributeDeclaration.uex' path='docs/doc[@for="CodeAttributeDeclaration.Arguments"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The arguments for the attribute.
        ///    </para>
        /// </devdoc>
        public CodeAttributeArgumentCollection Arguments {
            get {
                return arguments;
            }
        }
    }
}
