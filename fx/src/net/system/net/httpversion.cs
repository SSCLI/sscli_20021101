//------------------------------------------------------------------------------
// <copyright file="HttpVersion.cs" company="Microsoft">
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


    /// <include file='doc\HttpVersion.uex' path='docs/doc[@for="HttpVersion"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Defines the HTTP version number supported by the <see cref='System.Net.HttpWebRequest'/> and
    ///    <see cref='System.Net.HttpWebResponse'/> classes.
    ///    </para>
    /// </devdoc>
    public class HttpVersion {

        /// <include file='doc\HttpVersion.uex' path='docs/doc[@for="HttpVersion.Version10"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static readonly Version  Version10 = new Version(1,0);
        /// <include file='doc\HttpVersion.uex' path='docs/doc[@for="HttpVersion.Version11"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static readonly Version  Version11 = new Version(1,1);



    } // class HttpVersion


} // namespace System.Net
