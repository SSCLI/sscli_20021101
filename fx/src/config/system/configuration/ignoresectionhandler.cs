//------------------------------------------------------------------------------
// <copyright file="IgnoreSectionHandler.cs" company="Microsoft">
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

#if !LIB

namespace System.Configuration {

    /// <include file='doc\IgnoreSectionHandler.uex' path='docs/doc[@for="IgnoreSectionHandler"]/*' />
    /// <devdoc>
    /// </devdoc>
    public class IgnoreSectionHandler : IConfigurationSectionHandler {
        // Create
        //
        // Given a partially composed config object (possibly null)
        // and some input from the config system, return a
        // further partially composed config object
        /// <include file='doc\IgnoreSectionHandler.uex' path='docs/doc[@for="IgnoreSectionHandler.Create"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual object Create(Object parent, Object configContext, System.Xml.XmlNode section) {
            return null;
        }
    }
}

#endif

