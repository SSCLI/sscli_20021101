//------------------------------------------------------------------------------
// <copyright file="XPathNamespaceScope.cs" company="Microsoft">
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

namespace System.Xml.XPath 
{
    using System;

    /// <include file='doc\XPathNamespaceScope.uex' path='docs/doc[@for="XPathNamespaceScope"]/*' />
    public enum XPathNamespaceScope {
        /// <include file='doc\XPathNamespaceScope.uex' path='docs/doc[@for="XPathNamespaceScope.All"]/*' />
        All,
        /// <include file='doc\XPathNamespaceScope.uex' path='docs/doc[@for="XPathNamespaceScope.ExcludeXml"]/*' />
        ExcludeXml,
        /// <include file='doc\XPathNamespaceScope.uex' path='docs/doc[@for="XPathNamespaceScope.Local"]/*' />
        Local
    }
}
