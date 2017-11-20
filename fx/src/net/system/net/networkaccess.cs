//------------------------------------------------------------------------------
// <copyright file="NetworkAccess.cs" company="Microsoft">
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
 


namespace System.Net {


    /// <include file='doc\NetworkAccess.uex' path='docs/doc[@for="NetworkAccess"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Defines network access permissions.
    ///    </para>
    /// </devdoc>
    public  enum    NetworkAccess {
        /// <include file='doc\NetworkAccess.uex' path='docs/doc[@for="NetworkAccess.Accept"]/*' />
        /// <devdoc>
        ///    <para>
        ///       An application is allowed to accept connections from the Internet.
        ///    </para>
        /// </devdoc>
        Accept  = 0x80,
        /// <include file='doc\NetworkAccess.uex' path='docs/doc[@for="NetworkAccess.Connect"]/*' />
        /// <devdoc>
        ///    <para>
        ///       An application is allowed to connect to Internet resources.
        ///    </para>
        /// </devdoc>
        Connect = 0x40



} // enum NetworkAccess


} // namespace System.Net
