//------------------------------------------------------------------------------
// <copyright file="WhiteSpaceHandling.cs" company="Microsoft">
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

namespace System.Xml
{
    /// <include file='doc\WhiteSpaceHandling.uex' path='docs/doc[@for="WhitespaceHandling"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Specifies how whitespace is handled.
    ///    </para>
    /// </devdoc>
    public enum WhitespaceHandling
    {
        /// <include file='doc\WhiteSpaceHandling.uex' path='docs/doc[@for="WhitespaceHandling.All"]/*' />
        /// <devdoc>
        ///    Return Whitespace and SignificantWhitespace
        ///    only. This is the default.
        /// </devdoc>
        All              = 0,
        /// <include file='doc\WhiteSpaceHandling.uex' path='docs/doc[@for="WhitespaceHandling.Significant"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Return just SignificantWhitespace.
        ///    </para>
        /// </devdoc>
        Significant      = 1,
        /// <include file='doc\WhiteSpaceHandling.uex' path='docs/doc[@for="WhitespaceHandling.None"]/*' />
        /// <devdoc>
        ///    Return no Whitespace and no
        ///    SignificantWhitespace.
        /// </devdoc>
        None             = 2
    }
}
