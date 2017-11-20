//------------------------------------------------------------------------------
// <copyright file="XmlResolver.cs" company="Microsoft">
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
    using System;
    using System.Net;

    /// <include file='doc\XmlResolver.uex' path='docs/doc[@for="XmlResolver"]/*' />
    /// <devdoc>
    ///    <para>Resolves external XML resources named by a Uniform 
    ///       Resource Identifier (URI). This class is <see langword='abstract'/>
    ///       .</para>
    /// </devdoc>
    public abstract class XmlResolver {
        /// <include file='doc\XmlResolver.uex' path='docs/doc[@for="XmlResolver.GetEntity"]/*' />
        /// <devdoc>
        ///    <para>Maps a
        ///       URI to an Object containing the actual resource.</para>
        /// </devdoc>

        public abstract Object GetEntity(Uri absoluteUri, 
                                         string role, 
                                         Type ofObjectToReturn);

        /// <include file='doc\XmlResolver.uex' path='docs/doc[@for="XmlResolver.ResolveUri"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public abstract Uri ResolveUri(Uri baseUri, string relativeUri);

        //UE attension
        /// <include file='doc\XmlResolver.uex' path='docs/doc[@for="XmlResolver.Credentials"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public abstract ICredentials Credentials {
            set;
        }

    }
}
