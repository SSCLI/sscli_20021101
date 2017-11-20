//------------------------------------------------------------------------------
// <copyright file="CodeDOMProvider.cs" company="Microsoft">
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

namespace System.CodeDom.Compiler {
    using System;
    using System.CodeDom;
    using System.ComponentModel;
    using System.IO;
    using System.Security.Permissions;

    /// <include file='doc\CodeDOMProvider.uex' path='docs/doc[@for="CodeDomProvider"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [PermissionSet(SecurityAction.LinkDemand, Name="FullTrust")]
    [PermissionSet(SecurityAction.InheritanceDemand, Name="FullTrust")]
    public abstract class CodeDomProvider : Component {
    
        /// <include file='doc\CodeDOMProvider.uex' path='docs/doc[@for="CodeDomProvider.FileExtension"]/*' />
        /// <devdoc>
        ///    <para>Retrieves the default extension to use when saving files using this code dom provider.</para>
        /// </devdoc>
        public virtual string FileExtension {
            get {
                return string.Empty;
            }
        }

        /// <include file='doc\CodeDOMProvider.uex' path='docs/doc[@for="CodeDomProvider.LanguageOptions"]/*' />
        /// <devdoc>
        ///    <para>Returns flags representing language variations.</para>
        /// </devdoc>
        public virtual LanguageOptions LanguageOptions {
            get {
                return LanguageOptions.None;
            }
        }
        
        /// <include file='doc\CodeDOMProvider.uex' path='docs/doc[@for="CodeDomProvider.CreateGenerator"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public abstract ICodeGenerator CreateGenerator();

        /// <include file='doc\CodeDOMProvider.uex' path='docs/doc[@for="CodeDomProvider.CreateGenerator1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual ICodeGenerator CreateGenerator(TextWriter output) {
            return CreateGenerator();
        }

        /// <include file='doc\CodeDOMProvider.uex' path='docs/doc[@for="CodeDomProvider.CreateGenerator2"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual ICodeGenerator CreateGenerator(string fileName) {
            return CreateGenerator();
        }

        /// <include file='doc\CodeDOMProvider.uex' path='docs/doc[@for="CodeDomProvider.CreateCompiler"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public abstract ICodeCompiler CreateCompiler();
        
        /// <include file='doc\CodeDOMProvider.uex' path='docs/doc[@for="CodeDomProvider.CreateParser"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual ICodeParser CreateParser() {
            return null;
        }

    }
}

