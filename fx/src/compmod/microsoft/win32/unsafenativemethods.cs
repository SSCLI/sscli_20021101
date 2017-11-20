//------------------------------------------------------------------------------
// <copyright file="UnsafeNativeMethods.cs" company="Microsoft">
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

namespace Microsoft.Win32 {
    using System.Runtime.InteropServices;
    using System.Threading;
    using System;
    using System.Security.Permissions;
    using System.Collections;
    using System.IO;
    using System.Text;

    [
    System.Security.SuppressUnmanagedCodeSecurityAttribute()
    ]
    internal class UnsafeNativeMethods {

        [DllImport(ExternDll.Kernel32, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern IntPtr CreateFile(string lpFileName,int dwDesiredAccess,int dwShareMode, NativeMethods.SECURITY_ATTRIBUTES lpSecurityAttributes, int dwCreationDisposition,int dwFlagsAndAttributes,IntPtr hTemplateFile);
        [DllImport(ExternDll.Kernel32, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern int GetTempPath(int bufferLen, System.Text.StringBuilder buffer);
        [DllImport(ExternDll.Kernel32, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern bool CloseHandle(IntPtr handle);
        [DllImport(ExternDll.Kernel32, CharSet=System.Runtime.InteropServices.CharSet.Auto, SetLastError=true)]
        public extern static bool CreateProcess(string lpApplicationName, [In] StringBuilder lpCommandLine, NativeMethods.SECURITY_ATTRIBUTES lpProcessAttributes, NativeMethods.SECURITY_ATTRIBUTES lpThreadAttributes, bool bInheritHandles, int dwCreationFlags, IntPtr lpEnvironment, string lpCurrentDirectory, NativeMethods.STARTUPINFO lpStartupInfo, NativeMethods.PROCESS_INFORMATION lpProcessInformation);
        [DllImport(ExternDll.Kernel32, CharSet=System.Runtime.InteropServices.CharSet.Auto, SetLastError=true)]
        public extern static bool GetExitCodeProcess(IntPtr hProcess, ref int lpExitCode);
        [DllImport(ExternDll.Kernel32, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern IntPtr GetStdHandle(int type);

        [DllImport(ExternDll.Kernel32, EntryPoint="PAL_Random")]
        internal extern static bool Random(bool bStrong, 
                           [Out, MarshalAs(UnmanagedType.LPArray)] byte[] buffer, int length);                           
    }
}
