//------------------------------------------------------------------------------
// <copyright file="NetworkCredential.cs" company="Microsoft">
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

    using System.IO;
    using System.Runtime.InteropServices;
    using System.Security.Permissions;
    using System.Text;

    /// <include file='doc\NetworkCredential.uex' path='docs/doc[@for="NetworkCredential"]/*' />
    /// <devdoc>
    ///    <para>Provides credentials for password-based
    ///       authentication schemes such as basic, digest, NTLM and Kerberos.</para>
    /// </devdoc>
    public class NetworkCredential : ICredentials {

        private static SecurityPermission m_unmanagedPermission;
        private static EnvironmentPermission m_environmentUserNamePermission;
        private static EnvironmentPermission m_environmentDomainNamePermission;
        private string m_userName;
        private string m_password;
        private string m_domain;

        static NetworkCredential() {
            m_unmanagedPermission = new SecurityPermission(SecurityPermissionFlag.UnmanagedCode);
            m_environmentUserNamePermission = new EnvironmentPermission(EnvironmentPermissionAccess.Read, "USERNAME");
            m_environmentDomainNamePermission = new EnvironmentPermission(EnvironmentPermissionAccess.Read, "USERDOMAIN");
        }

        /// <include file='doc\NetworkCredential.uex' path='docs/doc[@for="NetworkCredential.NetworkCredential2"]/*' />
        public NetworkCredential() {
        }

        /// <include file='doc\NetworkCredential.uex' path='docs/doc[@for="NetworkCredential.NetworkCredential"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Net.NetworkCredential'/>
        ///       class with name and password set as specified.
        ///    </para>
        /// </devdoc>
        public NetworkCredential(string userName, string password)
        : this(userName, password, string.Empty) {
        }

        /// <include file='doc\NetworkCredential.uex' path='docs/doc[@for="NetworkCredential.NetworkCredential1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Net.NetworkCredential'/>
        ///       class with name and password set as specified.
        ///    </para>
        /// </devdoc>
        public NetworkCredential(string userName, string password, string domain) : this() {
            UserName = userName;
            Password = password;
            Domain = domain;
        }

        /// <include file='doc\NetworkCredential.uex' path='docs/doc[@for="NetworkCredential.UserName"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The user name associated with this credential.
        ///    </para>
        /// </devdoc>
        public string UserName {
            get {
                m_environmentUserNamePermission.Demand();

                GlobalLog.Print("NetworkCredential::get_UserName: returning \"" + m_userName + "\"");
 
                return m_userName;
            }
            set {
                m_userName = value;
                GlobalLog.Print("NetworkCredential::set_UserName: m_userName: \"" + m_userName + "\"" );
            }
        }

        /// <include file='doc\NetworkCredential.uex' path='docs/doc[@for="NetworkCredential.Password"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The password for the user name.
        ///    </para>
        /// </devdoc>
        public string Password {
            get {
                m_unmanagedPermission.Demand();

                GlobalLog.Print("NetworkCredential::get_Password: returning \"" + m_password + "\"");
                return m_password;
            }
            set {
                m_password = value;
                GlobalLog.Print("NetworkCredential::set_Password: m_password: \"" + m_password + "\"" );
            }
        }

        /// <include file='doc\NetworkCredential.uex' path='docs/doc[@for="NetworkCredential.Domain"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The machine name that verifies
        ///       the credentials. Usually this is the host machine.
        ///    </para>
        /// </devdoc>
        public string Domain {
            get {
                GlobalLog.Print("NetworkCredential::get_Domain: returning \"" + m_domain + "\"");
                return m_domain;                
            }
            set {
                m_domain = value;
                GlobalLog.Print("NetworkCredential::set_Domain: m_domain: \"" + m_domain + "\"" );
            }
        }

        /// <include file='doc\NetworkCredential.uex' path='docs/doc[@for="NetworkCredential.GetCredential"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns an instance of the NetworkCredential class for a Uri and
        ///       authentication type.
        ///    </para>
        /// </devdoc>
        public NetworkCredential GetCredential(Uri uri, String authType) {
            return this;
        }

    } // class NetworkCredential
} // namespace System.Net
