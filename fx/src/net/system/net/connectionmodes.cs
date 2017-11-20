//------------------------------------------------------------------------------
// <copyright file="ConnectionModes.cs" company="Microsoft">
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


// Used to indicate the mode of how we use a transport
// Not sure whether this is the right way to define an enum
/// <include file='doc\ConnectionModes.uex' path='docs/doc[@for="ConnectionModes"]/*' />
/// <devdoc>
///    <para>
///       Specifies the mode used to establish a connection with a server.
///    </para>
/// </devdoc>
    internal enum ConnectionModes {
        /// <include file='doc\ConnectionModes.uex' path='docs/doc[@for="ConnectionModes.Single"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Non-persistent, one request per connection.
        ///    </para>
        /// </devdoc>
        Single      ,       // Non-Persistent, one request per connection
        /// <include file='doc\ConnectionModes.uex' path='docs/doc[@for="ConnectionModes.Persistent"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Persistent connection, one request/response at a time.
        ///    </para>
        /// </devdoc>
        Persistent,         // Persistant, one request/response at a time
        /// <include file='doc\ConnectionModes.uex' path='docs/doc[@for="ConnectionModes.Pipeline"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Persistent connection, many requests/responses in order.
        ///    </para>
        /// </devdoc>
        Pipeline ,          // Persistant, many requests/responses in order
        /// <include file='doc\ConnectionModes.uex' path='docs/doc[@for="ConnectionModes.Mux"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Persistent connection, many requests/responses out of order.
        ///    </para>
        /// </devdoc>
        Mux                 // Persistant, many requests/responses out of order



    } // enum ConnectionModes


} // namespace System.Net
