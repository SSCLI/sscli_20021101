//------------------------------------------------------------------------------
// <copyright file="CodeNamespace.cs" company="Microsoft">
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
    using System.Runtime.Serialization;
    using System.Runtime.InteropServices;

    /// <include file='doc\CodeNamespace.uex' path='docs/doc[@for="CodeNamespace"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents a
    ///       namespace declaration.
    ///    </para>
    /// </devdoc>
    [
        Serializable,
    ]
    public class CodeNamespace: CodeObject {
        private string name;
        private CodeNamespaceImportCollection imports = new CodeNamespaceImportCollection();
        private CodeCommentStatementCollection comments = new CodeCommentStatementCollection();
        private CodeTypeDeclarationCollection classes = new CodeTypeDeclarationCollection();
        private CodeNamespaceCollection namespaces = new CodeNamespaceCollection();
        
        private int  populated = 0x0;
        private const int ImportsCollection = 0x1;
        private const int CommentsCollection = 0x2;
        private const int TypesCollection = 0x4;
        
        
        
       
        /// <include file='doc\CodeNamespace.uex' path='docs/doc[@for="CodeNamespace.PopulateComments"]/*' />
        /// <devdoc>
        ///    <para>
        ///       An event that will be fired the first time the Comments Collection is accessed.  
        ///    </para>
        /// </devdoc>
        public event EventHandler PopulateComments;
        
         /// <include file='doc\CodeNamespace.uex' path='docs/doc[@for="CodeNamespace.PopulateImports"]/*' />
         /// <devdoc>
        ///    <para>
        ///       An event that will be fired the first time the Imports Collection is accessed.  
        ///    </para>
        /// </devdoc>
        public event EventHandler PopulateImports;

        
        
        /// <include file='doc\CodeNamespace.uex' path='docs/doc[@for="CodeNamespace.PopulateTypes"]/*' />
        /// <devdoc>
        ///    <para>
        ///       An event that will be fired the first time the Types Collection is accessed.  
        ///    </para>
        /// </devdoc>
        public event EventHandler PopulateTypes;

        /// <include file='doc\CodeNamespace.uex' path='docs/doc[@for="CodeNamespace.CodeNamespace"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of <see cref='System.CodeDom.CodeNamespace'/>.
        ///    </para>
        /// </devdoc>
        public CodeNamespace() {
        }

        /// <include file='doc\CodeNamespace.uex' path='docs/doc[@for="CodeNamespace.CodeNamespace1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of <see cref='System.CodeDom.CodeNamespace'/> using the specified name.
        ///    </para>
        /// </devdoc>
        public CodeNamespace(string name) {
            Name = name;
        }

        private CodeNamespace(SerializationInfo info, StreamingContext context) {
        }

        /// <include file='doc\CodeNamespace.uex' path='docs/doc[@for="CodeNamespace.Types"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the collection of classes.
        ///    </para>
        /// </devdoc>
        public CodeTypeDeclarationCollection Types {
            get {
                if (0 == (populated & TypesCollection)) {
                    populated |= TypesCollection;
                    if (PopulateTypes != null) PopulateTypes(this, EventArgs.Empty);
                }
                return classes;
            }
        }

        /// <include file='doc\CodeNamespace.uex' path='docs/doc[@for="CodeNamespace.Imports"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the collection of namespace imports used by the represented
        ///       namespace.
        ///    </para>
        /// </devdoc>
        public CodeNamespaceImportCollection Imports {
            get {
                if (0 == (populated & ImportsCollection)) {
                    populated |= ImportsCollection;
                    if (PopulateImports != null) PopulateImports(this, EventArgs.Empty);
                }
                return imports;
            }
        }

        /// <include file='doc\CodeNamespace.uex' path='docs/doc[@for="CodeNamespace.Name"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the name of the namespace.
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
        
        

        /// <include file='doc\CodeNamespace.uex' path='docs/doc[@for="CodeNamespace.Comments"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the member comment collection members.
        ///    </para>
        /// </devdoc>
        public CodeCommentStatementCollection Comments {
            get {
                if (0 == (populated & CommentsCollection)) {
                    populated |= CommentsCollection;
                    if (PopulateComments != null) PopulateComments(this, EventArgs.Empty);
                }
                return comments;
            }
        }
    }
}
