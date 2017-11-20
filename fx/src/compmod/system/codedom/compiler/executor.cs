//------------------------------------------------------------------------------
// <copyright file="Executor.cs" company="Microsoft">
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

namespace System.CodeDom.Compiler {

    using System.Diagnostics;
    using System;
    using System.ComponentModel;
    using System.Text;
    using System.Threading;
    using System.IO;
    using System.Collections;
    using System.Collections.Specialized;
    using System.Reflection;
    using System.Runtime.InteropServices;
    using System.CodeDom;
    using System.Security;
    using System.Security.Permissions;
    using Microsoft.Win32;
    using System.Globalization;
    
    /// <include file='doc\Executor.uex' path='docs/doc[@for="Executor"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Provides command execution functions for the CodeDom compiler.
    ///    </para>
    /// </devdoc>
    [PermissionSet(SecurityAction.LinkDemand, Name="FullTrust")]
    [PermissionSet(SecurityAction.InheritanceDemand, Name="FullTrust")]
    public sealed class Executor {

        // How long (in milliseconds) do we wait for the program to terminate
        private const int ProcessTimeOut = 600000;

        private Executor() {
        }

        /// <include file='doc\Executor.uex' path='docs/doc[@for="Executor.GetRuntimeInstallDirectory"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the runtime install directory.
        ///    </para>
        /// </devdoc>
        internal static string GetRuntimeInstallDirectory() {

            // Get the path to mscorlib.dll
            string s = typeof(object).Module.FullyQualifiedName;

            // Remove the file part to get the directory
            return Directory.GetParent(s).ToString() + Path.DirectorySeparatorChar;
        }

        private static IntPtr CreateInheritedFile(string file) {
            NativeMethods.SECURITY_ATTRIBUTES sec_attribs = new NativeMethods.SECURITY_ATTRIBUTES();
            sec_attribs.nLength = Marshal.SizeOf(sec_attribs);
            sec_attribs.bInheritHandle = true;

            IntPtr handle = UnsafeNativeMethods.CreateFile(file,
                                    NativeMethods.GENERIC_WRITE,
                                    NativeMethods.FILE_SHARE_READ,
                                    sec_attribs,
                                    NativeMethods.CREATE_ALWAYS,
                                    NativeMethods.FILE_ATTRIBUTE_NORMAL,
                                    IntPtr.Zero);
            if (handle == NativeMethods.INVALID_HANDLE_VALUE) {
                throw new ExternalException(SR.GetString(SR.ExecFailedToCreate, file), Marshal.GetLastWin32Error());
            }

            return handle;
        }

        /// <include file='doc\Executor.uex' path='docs/doc[@for="Executor.ExecWait"]/*' />
        /// <devdoc>
        /// </devdoc>
        public static void ExecWait(string cmd, TempFileCollection tempFiles) {
            string outputName = null;
            string errorName = null;
            ExecWaitWithCapture(IntPtr.Zero, cmd, tempFiles, ref outputName, ref errorName);
        }

        /// <include file='doc\Executor.uex' path='docs/doc[@for="Executor.ExecWaitWithCapture"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static int ExecWaitWithCapture(string cmd, TempFileCollection tempFiles, ref string outputName, ref string errorName) {
            return ExecWaitWithCapture(IntPtr.Zero, cmd, Environment.CurrentDirectory, tempFiles, ref outputName, ref errorName);
        }

        /// <include file='doc\Executor.uex' path='docs/doc[@for="Executor.ExecWaitWithCapture1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static int ExecWaitWithCapture(string cmd, string currentDir, TempFileCollection tempFiles, ref string outputName, ref string errorName) {
            return ExecWaitWithCapture(IntPtr.Zero, cmd, currentDir, tempFiles, ref outputName, ref errorName);
        }

        /// <include file='doc\Executor.uex' path='docs/doc[@for="Executor.ExecWaitWithCapture2"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static int ExecWaitWithCapture(IntPtr userToken, string cmd, TempFileCollection tempFiles, ref string outputName, ref string errorName) {
            return ExecWaitWithCapture(userToken, cmd, Environment.CurrentDirectory, tempFiles, ref outputName, ref errorName);
        }

        internal static int ExecWaitWithCapture(IntPtr userToken, string cmd, TempFileCollection tempFiles, ref string outputName, ref string errorName, string trueCmdLine) {
            return ExecWaitWithCapture(userToken, cmd, Environment.CurrentDirectory, tempFiles, ref outputName, ref errorName, trueCmdLine);
        }

        /// <include file='doc\Executor.uex' path='docs/doc[@for="Executor.ExecWaitWithCapture4"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static int ExecWaitWithCapture(IntPtr userToken, string cmd, string currentDir, TempFileCollection tempFiles, ref string outputName, ref string errorName) {
            return ExecWaitWithCapture(userToken, cmd, Environment.CurrentDirectory, tempFiles, ref outputName, ref errorName, null);
        }

        /// <include file='doc\Executor.uex' path='docs/doc[@for="Executor.ExecWaitWithCapture3"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        internal static int ExecWaitWithCapture(IntPtr userToken, string cmd, string currentDir, TempFileCollection tempFiles, ref string outputName, ref string errorName, string trueCmdLine) {

            IntSecurity.UnmanagedCode.Demand();

            IntPtr output;
            IntPtr error;

            int retValue = 0;

            if (outputName == null || outputName.Length == 0)
                outputName = tempFiles.AddExtension("out");

            if (errorName == null || errorName.Length == 0)
                errorName = tempFiles.AddExtension("err");

            // Create the files
            output = CreateInheritedFile(outputName);
            error = CreateInheritedFile(errorName);

            bool success = false;
            NativeMethods.PROCESS_INFORMATION pi = new NativeMethods.PROCESS_INFORMATION();
            IntPtr primaryToken = IntPtr.Zero;
            GCHandle environmentHandle = new GCHandle();

            try {
                // Output the command line...
                // Make sure the FileStream doesn't own the handle
                FileStream outputStream = new FileStream(output, FileAccess.ReadWrite, false /*ownsHandle*/);
                StreamWriter sw = new StreamWriter(outputStream, Encoding.UTF8);
                sw.Write(currentDir);
                sw.Write("> ");
                // 'true' command line is used in case the command line points to
                // a response file                             
                sw.WriteLine(trueCmdLine != null ? trueCmdLine : cmd);
                sw.WriteLine();
                sw.WriteLine();
                sw.Flush();
                outputStream.Close();

                NativeMethods.STARTUPINFO si = new NativeMethods.STARTUPINFO();

                si.cb = Marshal.SizeOf(si);
                si.dwFlags = NativeMethods.STARTF_USESTDHANDLES;
                si.hStdOutput = output;
                si.hStdError = error;
                si.hStdInput = UnsafeNativeMethods.GetStdHandle(NativeMethods.STD_INPUT_HANDLE);

                //
                // Prepare the environment
                //
                Hashtable environment = new Hashtable();

                // Add the current environment
                foreach (DictionaryEntry entry in Environment.GetEnvironmentVariables())
                    environment.Add((string)entry.Key, (string)entry.Value);

                // Add the flag to indicate restricted security in the process
                environment.Add("_ClrRestrictSecAttributes", "1");

                // set up the environment block parameter
                IntPtr environmentPtr = (IntPtr)0;
                byte[] environmentBytes = EnvironmentToByteArray(environment);
                environmentHandle = GCHandle.Alloc(environmentBytes, GCHandleType.Pinned);
                environmentPtr = environmentHandle.AddrOfPinnedObject();

                if (userToken == IntPtr.Zero) {
                    success = UnsafeNativeMethods.CreateProcess(
                                                null,       // String lpApplicationName, 
                                                new StringBuilder(cmd), // String lpCommandLine, 
                                                null,       // SECURITY_ATTRIBUTES lpProcessAttributes, 
                                                null,       // SECURITY_ATTRIBUTES lpThreadAttributes, 
                                                true,       // bool bInheritHandles, 
                                                0,          // int dwCreationFlags, 
                                                environmentPtr, // int lpEnvironment, 
                                                currentDir, // String lpCurrentDirectory, 
                                                si,         // STARTUPINFO lpStartupInfo, 
                                                pi);        // PROCESS_INFORMATION lpProcessInformation);
                }
                else {
                    throw new NotSupportedException();

                }
            }
            finally {

                // free environment block
                if (environmentHandle.IsAllocated)
                    environmentHandle.Free();   

                // Close the file handles
                UnsafeNativeMethods.CloseHandle(output);
                UnsafeNativeMethods.CloseHandle(error);
            }

            if (success) {

                try {
                    int ret = SafeNativeMethods.WaitForSingleObject(pi.hProcess, ProcessTimeOut);

                    // Check for timeout
                    if (ret == NativeMethods.WAIT_TIMEOUT) {
                        throw new ExternalException(SR.GetString(SR.ExecTimeout, cmd), NativeMethods.WAIT_TIMEOUT);
                    }

                    if (ret != NativeMethods.WAIT_OBJECT_0) {
                        throw new ExternalException(SR.GetString(SR.ExecBadreturn, cmd), Marshal.GetLastWin32Error());
                    }

                    // Check the process's exit code
                    int status = NativeMethods.STILL_ACTIVE;
                    if (!UnsafeNativeMethods.GetExitCodeProcess(pi.hProcess, ref status)) {
                        throw new ExternalException(SR.GetString(SR.ExecCantGetRetCode, cmd), Marshal.GetLastWin32Error());
                    }

                    retValue = status;
                }
                finally {
                    UnsafeNativeMethods.CloseHandle(pi.hThread);
                    UnsafeNativeMethods.CloseHandle(pi.hProcess);

                    if (primaryToken != IntPtr.Zero)
                        UnsafeNativeMethods.CloseHandle(primaryToken);
                }
            }
            else {
                throw new ExternalException(SR.GetString(SR.ExecCantExec, cmd), Marshal.GetLastWin32Error());
            }

            return retValue;
        }

        private static byte[] EnvironmentToByteArray(Hashtable hash) {
            // get the keys
            string[] keys = new string[hash.Count];
            hash.Keys.CopyTo(keys, 0);
            
            // get the values
            string[] values = new string[hash.Count];
            hash.Values.CopyTo(values, 0);
            
            // sort both by the keys
            Array.Sort(keys, values, InvariantComparer.Default);

            // create a list of null terminated "key=val" strings
            StringBuilder stringBuff = new StringBuilder();
            for (int i = 0; i < hash.Count; ++ i) {
                stringBuff.Append(keys[i]);
                stringBuff.Append('=');
                stringBuff.Append(values[i]);
                stringBuff.Append('\0');
            }
            // an extra null at the end indicates end of list.
            stringBuff.Append('\0');
            
            int byteCount = stringBuff.Length;
            byte[] bytes = Encoding.Default.GetBytes(stringBuff.ToString());
                        
            return bytes;
        }

    }
}

