//------------------------------------------------------------------------------
// <copyright file="CodeNamespaceImport.cs" company="Microsoft">
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

    /// <include file='doc\CodeNamespaceImport.uex' path='docs/doc[@for="CodeNamespaceImport"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents a namespace import into the current namespace.
    ///    </para>
    /// </devdoc>
    [
        Serializable,
    ]
    public class CodeNamespaceImport : CodeObject {
        private string nameSpace;
        private CodeLinePragma linePragma;

        /// <include file='doc\CodeNamespaceImport.uex' path='docs/doc[@for="CodeNamespaceImport.CodeNamespaceImport"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of <see cref='System.CodeDom.CodeNamespaceImport'/>.
        ///    </para>
        /// </devdoc>
        public CodeNamespaceImport() {
        }

        /// <include file='doc\CodeNamespaceImport.uex' path='docs/doc[@for="CodeNamespaceImport.CodeNamespaceImport1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of <see cref='System.CodeDom.CodeNamespaceImport'/> using the specified namespace
        ///       to import.
        ///    </para>
        /// </devdoc>
        public CodeNamespaceImport(string nameSpace) {
            Namespace = nameSpace;
        }

        /// <include file='doc\CodeNamespaceImport.uex' path='docs/doc[@for="CodeNamespaceImport.LinePragma"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The line the statement occurs on.
        ///    </para>
        /// </devdoc>
        public CodeLinePragma LinePragma {
            get {
                return linePragma;
            }
            set {
                linePragma = value;
            }
        }

        /// <include file='doc\CodeNamespaceImport.uex' path='docs/doc[@for="CodeNamespaceImport.Namespace"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets
        ///       the namespace to import.
        ///    </para>
        /// </devdoc>
        public string Namespace {
            get {
                return (nameSpace == null) ? string.Empty : nameSpace;
            }
            set {
                nameSpace = value;
            }
        }
    }
}
