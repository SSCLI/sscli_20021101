//------------------------------------------------------------------------------
// <copyright file="XmlSpace.cs" company="Microsoft">
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
    /// <include file='doc\XmlSpace.uex' path='docs/doc[@for="XmlSpace"]/*' />
    /// <devdoc>
    ///    Specifies the current xml:space scope.
    /// </devdoc>
    public enum XmlSpace
    {
        /// <include file='doc\XmlSpace.uex' path='docs/doc[@for="XmlSpace.None"]/*' />
        /// <devdoc>
        ///    No xml:space scope.
        /// </devdoc>
        None          = 0,
        /// <include file='doc\XmlSpace.uex' path='docs/doc[@for="XmlSpace.Default"]/*' />
        /// <devdoc>
        ///    The xml:space scope equals "default".
        /// </devdoc>
        Default       = 1,
        /// <include file='doc\XmlSpace.uex' path='docs/doc[@for="XmlSpace.Preserve"]/*' />
        /// <devdoc>
        ///    The xml:space scope equals "preserve".
        /// </devdoc>
        Preserve      = 2
    }
}
