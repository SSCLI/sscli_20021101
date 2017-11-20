//------------------------------------------------------------------------------
// <copyright file="XmlUrlResolver.cs" company="Microsoft">
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
    using System.IO;
    using System.Net;
    using System.Text;

    /// <include file='doc\XmlUrlResolver.uex' path='docs/doc[@for="XmlUrlResolver"]/*' />
    /// <devdoc>
    ///    <para>Resolves external XML resources named by a
    ///       Uniform Resource Identifier (URI).</para>
    /// </devdoc>
    public class XmlUrlResolver : XmlResolver {
        private const int NOTPREFIXED         = -1;
        private const int PREFIXED            =  1;
        private const int ABSOLUTENOTPREFIXED =  2;
        private const int SYSTEMROOTMISSING   =  3;

        static XmlDownloadManager _DownloadManager = new XmlDownloadManager();
        ICredentials _credentials;

        // Construction

        /// <include file='doc\XmlUrlResolver.uex' path='docs/doc[@for="XmlUrlResolver.XmlUrlResolver"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates a new instance of the XmlUrlResolver class.
        ///    </para>
        /// </devdoc>
        public XmlUrlResolver() {
        }

        //UE attension
        /// <include file='doc\XmlUrlResolver.uex' path='docs/doc[@for="XmlUrlResolver.Credentials"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override ICredentials Credentials {
            set { _credentials = value; }
        }

        // Resource resolution

        /// <include file='doc\XmlUrlResolver.uex' path='docs/doc[@for="XmlUrlResolver.GetEntity"]/*' />
        /// <devdoc>
        ///    <para>Maps a
        ///       URI to an Object containing the actual resource.</para>
        /// </devdoc>
        public override Object GetEntity(Uri absoluteUri,
                                         string role,
                                         Type ofObjectToReturn) {
            if (ofObjectToReturn == null || ofObjectToReturn == typeof(System.IO.Stream)) {
                return _DownloadManager.GetStream(absoluteUri, _credentials);
            }
            else {
                throw new XmlException(Res.Xml_UnsupportedClass);
            }
        }

        /// <include file='doc\XmlUrlResolver.uex' path='docs/doc[@for="XmlUrlResolver.ResolveUri"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override Uri ResolveUri(Uri baseUri, string relativeUri) {
            if (null == relativeUri) {
                relativeUri = String.Empty;
            }
            int prefixed = IsPrefixed(relativeUri);
            if (prefixed == PREFIXED) {
                if (relativeUri.StartsWith("file:"))
                    return new Uri(Escape(relativeUri), true);
                else
                    return new Uri(relativeUri);
            }
            else if (prefixed == ABSOLUTENOTPREFIXED) {
                if (relativeUri.StartsWith("file:"))
                    return new Uri(Escape(relativeUri), true);
                else
                    return new Uri(Escape("file://" + relativeUri), true);
            }
            else if (prefixed == SYSTEMROOTMISSING) {
                // we have gotten a path like "/foo/bar.xml" we should use the drive letter from the baseUri if available.
                if (null == baseUri)
                    return new Uri(Escape(Path.GetFullPath(relativeUri)), true);
                else
                    if ("file" == baseUri.Scheme) {
                        return new Uri(Escape(Path.GetFullPath(Path.GetPathRoot(baseUri.LocalPath) + relativeUri)), true);
                    }
                    return new Uri(baseUri, relativeUri);
            }
            else if (baseUri != null) {
                if (baseUri.Scheme == "file"){
                    baseUri = new Uri(Escape(baseUri.ToString()), true);   
                    return new Uri(baseUri, Escape(relativeUri), true);
                }   
                else
                    return new Uri(baseUri, relativeUri);
            }
            else {
                return new Uri(Escape(Path.GetFullPath(relativeUri)), true);
            }
        }

        private static string Escape(string path) {
            StringBuilder b = new StringBuilder();
            char c;
            for (int i = 0; i < path.Length; i++) {
                c = path[i];
                if (c == '\\')
                    b.Append('/');
                else if (c == '#')
                    b.Append("%23");
                else
                    b.Append(c);
            }
            return b.ToString();
        }

        internal static string UnEscape(string path) {
        if (null != path && path.StartsWith("file")) {
                return path.Replace("%23", "#");
        }
        return path;
        }


        private static int IsPrefixed(string uri) {
            if (uri != null && uri.IndexOf("://") >= 0)
                return PREFIXED;
            else if (uri.IndexOf(":\\") > 0)
                return ABSOLUTENOTPREFIXED;
            else if (uri.Length > 1 && uri[0] == '\\' && uri[1] != '\\')
                return SYSTEMROOTMISSING;
            else
                return NOTPREFIXED;
        }
    }
}
