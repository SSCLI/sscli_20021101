//------------------------------------------------------------------------------
// <copyright file="CodeCompileUnit.cs" company="Microsoft">
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
    using System.Collections.Specialized;
    using System.Runtime.Serialization;
    using System.Runtime.InteropServices;

    /// <include file='doc\CodeCompileUnit.uex' path='docs/doc[@for="CodeCompileUnit"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents a
    ///       compilation unit declaration.
    ///    </para>
    /// </devdoc>
    [
        Serializable,
    ]
    public class CodeCompileUnit: CodeObject {
        private CodeNamespaceCollection namespaces = new CodeNamespaceCollection();
        private StringCollection assemblies = null;
        private CodeAttributeDeclarationCollection attributes = null;

        /// <include file='doc\CodeCompileUnit.uex' path='docs/doc[@for="CodeCompileUnit.CodeCompileUnit"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of <see cref='System.CodeDom.CodeCompileUnit'/>.
        ///    </para>
        /// </devdoc>
        public CodeCompileUnit() {
        }


        /// <include file='doc\CodeCompileUnit.uex' path='docs/doc[@for="CodeCompileUnit.Namespaces"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the collection of namespaces.
        ///    </para>
        /// </devdoc>
        public CodeNamespaceCollection Namespaces {
            get {
                return namespaces;
            }
        }

        /// <include file='doc\CodeCompileUnit.uex' path='docs/doc[@for="CodeCompileUnit.ReferencedAssemblies"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the collection of assemblies. Most code generators will not need this, but the Managed
        ///       extensions for C++ code generator and 
        ///       other very low level code generators will need to do a more complete compilation. If both this
        ///       and the compiler assemblies are specified, the compiler assemblies should win.
        ///    </para>
        /// </devdoc>
        public StringCollection ReferencedAssemblies {
            get {
                if (assemblies == null) {
                    assemblies = new StringCollection();
                }
                return assemblies;
            }
        }

        /// <include file='doc\CodeCompileUnit.uex' path='docs/doc[@for="CodeCompileUnit.AssemblyCustomAttributes"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the collection of assembly level attributes.
        ///    </para>
        /// </devdoc>
        public CodeAttributeDeclarationCollection AssemblyCustomAttributes {
            get {
                if (attributes == null) {
                    attributes = new CodeAttributeDeclarationCollection();
                }
                return attributes;
            }
        }
    }
}
