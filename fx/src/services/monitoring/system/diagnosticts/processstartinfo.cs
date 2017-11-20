//------------------------------------------------------------------------------
// <copyright file="ProcessStartInfo.cs" company="Microsoft">
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

namespace System.Diagnostics {
    using System.Threading;
    using System.Runtime.InteropServices;
    using System.ComponentModel;
    using System.Diagnostics;
    using System;
    using System.Security;
    using System.Security.Permissions;
    using Microsoft.Win32;
    using System.IO;   
    using System.Collections.Specialized;
    using System.Collections;
    using System.Globalization;
    

    /// <include file='doc\ProcessStartInfo.uex' path='docs/doc[@for="ProcessStartInfo"]/*' />
    /// <devdoc>
    ///     A set of values used to specify a process to start.  This is
    ///     used in conjunction with the <see cref='System.Diagnostics.Process'/>
    ///     component.
    /// </devdoc>

    [
    // Disabling partial trust scenarios
    PermissionSet(SecurityAction.LinkDemand, Name="FullTrust")
    ]
    public sealed class ProcessStartInfo {
        string fileName;
        string arguments;
        string directory;
        internal StringDictionary environmentVariables;
        bool redirectStandardInput = false;
        bool redirectStandardOutput = false;
        bool redirectStandardError = false;
        bool useShellExecute = false;

        /// <include file='doc\ProcessStartInfo.uex' path='docs/doc[@for="ProcessStartInfo.ProcessStartInfo"]/*' />
        /// <devdoc>
        ///     Default constructor.  At least the <see cref='System.Diagnostics.ProcessStartInfo.FileName'/>
        ///     property must be set before starting the process.
        /// </devdoc>
        public ProcessStartInfo() {
        }

        internal ProcessStartInfo(Process parent) {
        }

        /// <include file='doc\ProcessStartInfo.uex' path='docs/doc[@for="ProcessStartInfo.ProcessStartInfo1"]/*' />
        /// <devdoc>
        ///     Specifies the name of the application or document that is to be started.
        /// </devdoc>
        public ProcessStartInfo(string fileName) {
            this.fileName = fileName;
        }

        /// <include file='doc\ProcessStartInfo.uex' path='docs/doc[@for="ProcessStartInfo.ProcessStartInfo2"]/*' />
        /// <devdoc>
        ///     Specifies the name of the application that is to be started, as well as a set
        ///     of command line arguments to pass to the application.
        /// </devdoc>
        public ProcessStartInfo(string fileName, string arguments) {
            this.fileName = fileName;
            this.arguments = arguments;
        }

        /// <include file='doc\ProcessStartInfo.uex' path='docs/doc[@for="ProcessStartInfo.Arguments"]/*' />
        /// <devdoc>
        ///     Specifies the set of command line arguments to use when starting the application.
        /// </devdoc>
        public string Arguments {
            get {
                if (arguments == null) return string.Empty;
                return arguments;
            }
            set {
                arguments = value;
            }
        }


        /// <include file='doc\ProcessStartInfo.uex' path='docs/doc[@for="ProcessStartInfo.EnvironmentVariables"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public StringDictionary EnvironmentVariables {
            get { 
                if (environmentVariables == null) {
                    environmentVariables = new StringDictionary();

                    {
                        foreach (DictionaryEntry entry in Environment.GetEnvironmentVariables())
                            environmentVariables.Add((string)entry.Key, (string)entry.Value);
                    }
                }
                return environmentVariables; 
            }                        
        }

        /// <include file='doc\ProcessStartInfo.uex' path='docs/doc[@for="ProcessStartInfo.RedirectStandardInput"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public bool RedirectStandardInput {
            get { return redirectStandardInput; }
            set { redirectStandardInput = value; }
        }
        
        /// <include file='doc\ProcessStartInfo.uex' path='docs/doc[@for="ProcessStartInfo.RedirectStandardOutput"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public bool RedirectStandardOutput {
            get { return redirectStandardOutput; }
            set { redirectStandardOutput = value; }
        }
        
        /// <include file='doc\ProcessStartInfo.uex' path='docs/doc[@for="ProcessStartInfo.RedirectStandardError"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public bool RedirectStandardError {
            get { return redirectStandardError; }
            set { redirectStandardError = value; }
        }

        /// <include file='doc\ProcessStartInfo.uex' path='docs/doc[@for="ProcessStartInfo.FileName"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns or sets the application, document, or URL that is to be launched.
        ///    </para>
        /// </devdoc>
        public string FileName {
            get {
                if (fileName == null) return string.Empty;
                return fileName;
            }
            set { fileName = value;}
        }

        /// <include file='doc\ProcessStartInfo.uex' path='docs/doc[@for="ProcessStartInfo.WorkingDirectory"]/*' />
        /// <devdoc>
        ///     Returns or sets the initial directory for the process that is started.
        ///     Specify "" to if the default is desired.
        /// </devdoc>
        public string WorkingDirectory {
            get {
                if (directory == null) return string.Empty;
                return directory;
            }
            set {
                directory = value;
            }
        }

        /// <include file='doc\ProcessStartInfo.uex' path='docs/doc[@for="ProcessStartInfo.UseShellExecute"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public bool UseShellExecute {
            get { return useShellExecute; }
            set { useShellExecute = value; }
        }

    }
}
