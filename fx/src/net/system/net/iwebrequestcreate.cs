//------------------------------------------------------------------------------
// <copyright file="IWebRequestCreate.cs" company="Microsoft">
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


//
// IWebRequestCreate - Interface for creating WebRequests.
//

    /// <include file='doc\IWebRequestCreate.uex' path='docs/doc[@for="IWebRequestCreate"]/*' />
    /// <devdoc>
    ///    <para>
    ///       The <see cref='System.Net.IWebRequestCreate'/> interface is used by the <see cref='System.Net.WebRequest'/>
    ///       class to create <see cref='System.Net.WebRequest'/>
    ///       instances for a registered scheme.
    ///    </para>
    /// </devdoc>
    public interface IWebRequestCreate {
        /// <include file='doc\IWebRequestCreate.uex' path='docs/doc[@for="IWebRequestCreate.Create"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates a <see cref='System.Net.WebRequest'/>
        ///       instance.
        ///    </para>
        /// </devdoc>
        WebRequest Create(Uri uri);
    } // interface IWebRequestCreate


} // namespace System.Net
