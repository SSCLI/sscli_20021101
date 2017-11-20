//------------------------------------------------------------------------------
// <copyright file="iwebproxy.cs" company="Microsoft">
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


namespace System.Net {
    using System.Runtime.Serialization;

    /// <include file='doc\iwebproxy.uex' path='docs/doc[@for="IWebProxy"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Holds the interface for implementation of the proxy interface.
    ///       Used to implement and control proxy use of WebRequests. 
    ///    </para>
    /// </devdoc>
    public interface IWebProxy  { 
        /// <include file='doc\iwebproxy.uex' path='docs/doc[@for="IWebProxy.GetProxy"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>

        Uri GetProxy( Uri destination );
        /// <include file='doc\iwebproxy.uex' path='docs/doc[@for="IWebProxy.IsBypassed"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        bool IsBypassed(Uri host);
        /// <include file='doc\iwebproxy.uex' path='docs/doc[@for="IWebProxy.Credentials"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        ICredentials Credentials { get; set; }
    }
}
