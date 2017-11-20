//------------------------------------------------------------------------------
// <copyright file="LanguageOptions.cs" company="Microsoft">
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
    
    /// <include file='doc\LanguageOptions.uex' path='docs/doc[@for="LanguageOptions"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [
        Flags,
        Serializable,
    ]
    public enum LanguageOptions {
        /// <include file='doc\LanguageOptions.uex' path='docs/doc[@for="LanguageOptions.None"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        None = 0x0,
        /// <include file='doc\LanguageOptions.uex' path='docs/doc[@for="LanguageOptions.CaseInsensitive"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        CaseInsensitive = 0x1,
    }
}
