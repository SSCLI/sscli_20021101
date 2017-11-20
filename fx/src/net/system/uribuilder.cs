//------------------------------------------------------------------------------
// <copyright file="uribuilder.cs" company="Microsoft">
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

namespace System {

    using System.Text;
    using System.Globalization;
    
    /// <include file='doc\uribuilder.uex' path='docs/doc[@for="UriBuilder"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public class UriBuilder {

    // fields

        private bool m_changed = true;
        private string m_fragment = String.Empty;
        private string m_host = "loopback";
        private string m_password = String.Empty;
        private string m_path = "/";
        private int m_port = -1;
        private string m_query = String.Empty;
        private string m_scheme = "http";
        private string m_schemeDelimiter = "://";
        private Uri m_uri;
        private string m_username = String.Empty;

    // constructors

        /// <include file='doc\uribuilder.uex' path='docs/doc[@for="UriBuilder.UriBuilder"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public UriBuilder() {
        }

        /// <include file='doc\uribuilder.uex' path='docs/doc[@for="UriBuilder.UriBuilder1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public UriBuilder(string uri) {
            
            bool supplyHttpDefault = false;
            int  schemeEnd = uri.IndexOf(':');

            if (schemeEnd == -1) {   //no scheme
                supplyHttpDefault = true;
            }
            else if (schemeEnd > 0) {
                string scheme = uri.Substring(0,schemeEnd);
                if (Uri.SchemeHasSlashes(scheme)) { //exclude mailto/news
                    schemeEnd = uri.IndexOf(Uri.SchemeDelimiter);
                    if (schemeEnd == -1) {
                        supplyHttpDefault = true;
                    }
                }
            }

            if (supplyHttpDefault) {
                uri = Uri.UriSchemeHttp + Uri.SchemeDelimiter + uri;
            }
            
            Init(new Uri(uri));
        }

        /// <include file='doc\uribuilder.uex' path='docs/doc[@for="UriBuilder.UriBuilder2"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public UriBuilder(Uri uri) {
            Init(uri);
        }

        private void Init(Uri uri) {
            if (uri.Fragment != String.Empty) {
                m_fragment = uri.Fragment;
            }
            else {
                m_query = uri.Query;
            }
            m_host = uri.Host;
            m_path = uri.AbsolutePath;
            m_port = uri.Port;
            m_scheme = uri.Scheme;
            m_schemeDelimiter = Uri.SchemeHasSlashes(m_scheme) ? "://" : ":";
            //m_uri = (Uri)uri.MemberwiseClone();

            string userInfo = uri.UserInfo;

            if (userInfo != String.Empty) {

                int index = userInfo.IndexOf(':');

                if (index != -1) {
                    m_password = userInfo.Substring(index + 1);
                    m_username = userInfo.Substring(0, index);
                }
                else {
                    m_username = userInfo;
                }
            }
            SetFieldsFromUri(uri);
        }

        /// <include file='doc\uribuilder.uex' path='docs/doc[@for="UriBuilder.UriBuilder3"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public UriBuilder(string schemeName, string hostName) {
            Scheme = schemeName;
            Host = hostName;
        }

        /// <include file='doc\uribuilder.uex' path='docs/doc[@for="UriBuilder.UriBuilder4"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public UriBuilder(string scheme, string host, int portNumber) : this(scheme, host) {
            try {
                Port = portNumber;
            }
            catch {
                throw new ArgumentOutOfRangeException("portNumber");
            }
        }

        /// <include file='doc\uribuilder.uex' path='docs/doc[@for="UriBuilder.UriBuilder5"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public UriBuilder(string scheme,
                          string host,
                          int port,
                          string pathValue
                          ) : this(scheme, host, port)
        {
            bool escaped;
            Path = Uri.EscapeString(ConvertSlashes(pathValue), false, out escaped);
        }

        /// <include file='doc\uribuilder.uex' path='docs/doc[@for="UriBuilder.UriBuilder6"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public UriBuilder(string scheme,
                          string host,
                          int port,
                          string path,
                          string extraValue
                          ) : this(scheme, host, port, path)
        {
            try {
                Extra = extraValue;
            }
            catch {
                throw new ArgumentException("extraValue");
            }
        }

    // properties

        private string Extra {
            get {
                if (m_fragment.Length > 0) {
                    return Fragment;
                }
                else {
                    return Query;
                }
            }
            set {
                if (value == null) {
                    value = String.Empty;
                }
                if (value.Length > 0) {
                    if (value[0] == '#') {
                        Fragment = value.Substring(1);
                    }
                    else if (value[0] == '?') {
                        Query = value.Substring(1);
                    } else {
                        throw new ArgumentException("value");
                    }
                }
                else {
                    Fragment = String.Empty;
                    Query = String.Empty;
                }
            }
        }

        /// <include file='doc\uribuilder.uex' path='docs/doc[@for="UriBuilder.Fragment"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string Fragment {
            get {
                return m_fragment;
            }
            set {
                if (value == null) {
                    value = String.Empty;
                }
                if (value.Length > 0) {
                    value = '#' + value;
                }
                m_fragment = value;
                m_query = String.Empty;
                m_changed = true;
            }
        }

        /// <include file='doc\uribuilder.uex' path='docs/doc[@for="UriBuilder.Host"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string Host {
            get {
                return m_host;
            }
            set {
                if (value == null) {
                    value = String.Empty;
                }
                m_host = value;
                m_changed = true;
            }
        }

        /// <include file='doc\uribuilder.uex' path='docs/doc[@for="UriBuilder.Password"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string Password {
            get {
                return m_password;
            }
            set {
                if (value == null) {
                    value = String.Empty;
                }
                m_password = value;
            }
        }

        /// <include file='doc\uribuilder.uex' path='docs/doc[@for="UriBuilder.Path"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string Path {
            get {
                return m_path;
            }
            set {
                if ((value == null) || (value.Length == 0)) {
                    value = "/";
                }
                //if ((value[0] != '/') && (value[0] != '\\')) {
                //    value = '/' + value;
                //}
                bool escaped;
                m_path = Uri.EscapeString(ConvertSlashes(value), false, out escaped);
                m_changed = true;
            }
        }

        /// <include file='doc\uribuilder.uex' path='docs/doc[@for="UriBuilder.Port"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int Port {
            get {
                return m_port;
            }
            set {
                if (value < 0) {
                    throw new ArgumentOutOfRangeException("value");
                }
                m_port = value;
                m_changed = true;
            }
        }

        /// <include file='doc\uribuilder.uex' path='docs/doc[@for="UriBuilder.Query"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string Query {
            get {
                return m_query;
            }
            set {
                if (value == null) {
                    value = String.Empty;
                }
                if (value.Length > 0) {
                    value = '?' + value;
                }
                m_query = value;
                m_fragment = String.Empty;
                m_changed = true;
            }
        }

        /// <include file='doc\uribuilder.uex' path='docs/doc[@for="UriBuilder.Scheme"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string Scheme {
            get {
                return m_scheme;
            }
            set {
                if (value == null) {
                    value = String.Empty;
                }

                int index = value.IndexOf(':');

                if (index != -1) {
                    value = value.Substring(0, index);
                }
                m_scheme = value.ToLower(CultureInfo.InvariantCulture);
                m_schemeDelimiter = Uri.SchemeHasSlashes(value) ? "://" : ":";
                m_changed = true;
            }
        }

        /// <include file='doc\uribuilder.uex' path='docs/doc[@for="UriBuilder.Uri"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public Uri Uri {
            get {
                if (m_changed) {
                    m_uri = new Uri(ToString());
                    SetFieldsFromUri(m_uri);
                    m_changed = false;
                }
                return m_uri;
            }
        }

        /// <include file='doc\uribuilder.uex' path='docs/doc[@for="UriBuilder.UserName"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string UserName {
            get {
                return m_username;
            }
            set {
                if (value == null) {
                    value = String.Empty;
                }
                m_username = value;
            }
        }

    // methods

        private string ConvertSlashes(string path) {

            StringBuilder sb = new StringBuilder(path.Length);
            char ch;

            for (int i = 0; i < path.Length; ++i) {
                ch = path[i];
                if (ch == '\\') {
                    ch = '/';
                }
                sb.Append(ch);
            }
            return sb.ToString();
        }

        /// <include file='doc\uribuilder.uex' path='docs/doc[@for="UriBuilder.Equals"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override bool Equals(object rparam) {
            if (rparam == null) {
                return false;
            }
            return Uri.Equals(rparam.ToString());
        }

        /// <include file='doc\uribuilder.uex' path='docs/doc[@for="UriBuilder.GetHashCode"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override int GetHashCode() {
            return Uri.GetHashCode();
        }

        private void SetFieldsFromUri(Uri uri) {
            if (uri.Fragment.Length > 0) {
                m_fragment = uri.Fragment;
            }
            else {
                m_query = uri.Query;
            }
            m_host = uri.Host;
            m_path = uri.AbsolutePath;
            m_port = uri.Port;
            m_scheme = uri.Scheme;
            m_schemeDelimiter = Uri.SchemeHasSlashes(m_scheme) ? "://" : ":";
            //m_uri = (Uri)uri.MemberwiseClone();

            string userInfo = uri.UserInfo;

            if (userInfo.Length > 0) {

                int index = userInfo.IndexOf(':');

                if (index != -1) {
                    m_password = userInfo.Substring(index + 1);
                    m_username = userInfo.Substring(0, index);
                }
                else {
                    m_username = userInfo;
                }
            }
        }

        /// <include file='doc\uribuilder.uex' path='docs/doc[@for="UriBuilder.ToString"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override string ToString() {
            return m_scheme
                    + m_schemeDelimiter
                    + m_host
                    + (((m_port != -1) && (m_host.Length > 0)) ? (":" + m_port) : String.Empty)
                    + (((m_host.Length > 0) && (m_path.Length != 0) && (m_path[0] != '/')) ? "/" : String.Empty) + m_path
                    + ((m_fragment.Length > 0) ? m_fragment : m_query);
        }
    }
}
