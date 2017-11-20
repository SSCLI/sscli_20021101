//------------------------------------------------------------------------------
// <copyright file="MulticastOption.cs" company="Microsoft">
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

namespace System.Net.Sockets {
    using System;
    using System.Collections;
    using System.Configuration;
    using System.Configuration.Assemblies;
    using System.Diagnostics;
    using System.Globalization;
    using System.IO;
    using System.Net;
    using System.Net.Sockets;
    using System.Reflection;
    using System.Reflection.Emit;
    using System.Resources;
    using System.Runtime.InteropServices;
    using System.Runtime.Serialization;
    using System.Runtime.Serialization.Formatters;
    using System.Security;
    using System.Security.Permissions;
    using System.Security.Util;
    using System.Text;
    using System.Text.RegularExpressions;
    using System.Threading;


    /// <include file='doc\MulticastOption.uex' path='docs/doc[@for="MulticastOption"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Contains option values
    ///       for IP multicast packets.
    ///    </para>
    /// </devdoc>
    public class MulticastOption {
        IPAddress group;
        IPAddress localAddress;

        /// <include file='doc\MulticastOption.uex' path='docs/doc[@for="MulticastOption.MulticastOption"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates a new instance of the MulticaseOption class with the specified IP
        ///       address group and local address.
        ///    </para>
        /// </devdoc>
        public MulticastOption(IPAddress group, IPAddress mcint) {

            if (group == null) {
                throw new ArgumentNullException("group");
            }

            if (mcint == null) {
                throw new ArgumentNullException("mcint");
            }

            Group = group;
            LocalAddress = mcint;
        }

        /// <include file='doc\MulticastOption.uex' path='docs/doc[@for="MulticastOption.MulticastOption1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates a new version of the MulticastOption class for the specified
        ///       group.
        ///    </para>
        /// </devdoc>
        public MulticastOption(IPAddress group) {

            if (group == null) {
                throw new ArgumentNullException("group");
            }

            Group = group;

            LocalAddress = IPAddress.Any;
        }
        
        /// <include file='doc\MulticastOption.uex' path='docs/doc[@for="MulticastOption.Group"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Sets the IP address of a multicast group.
        ///    </para>
        /// </devdoc>
        public IPAddress Group {
            get {
                return group;
            }
            set {
                group = value;
            }
        }

        /// <include file='doc\MulticastOption.uex' path='docs/doc[@for="MulticastOption.LocalAddress"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Sets the local address of a multicast group.
        ///    </para>
        /// </devdoc>
        public IPAddress LocalAddress {
            get {
                return localAddress;
            }
            set {
                localAddress = value;
            }
        }

    } // class MulticastOption
} // namespace System.Net.Sockets

