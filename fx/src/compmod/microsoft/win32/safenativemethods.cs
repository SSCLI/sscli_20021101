//------------------------------------------------------------------------------
// <copyright file="SafeNativeMethods.cs" company="Microsoft">
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
    using System;
    using System.Security.Permissions;
    using System.Collections;
    using System.IO;
    using System.Text;

    [
    System.Security.SuppressUnmanagedCodeSecurityAttribute()
    ]
    internal class SafeNativeMethods {

        public const int
            FORMAT_MESSAGE_ALLOCATE_BUFFER = 0x00000100,
            FORMAT_MESSAGE_IGNORE_INSERTS = 0x00000200,
            FORMAT_MESSAGE_FROM_STRING = 0x00000400,
            FORMAT_MESSAGE_FROM_SYSTEM = 0x00001000,
            FORMAT_MESSAGE_ARGUMENT_ARRAY = 0x00002000;

        [DllImport(ExternDll.Kernel32, CharSet=System.Runtime.InteropServices.CharSet.Auto, SetLastError=true)]
        public static extern int FormatMessage(int dwFlags, IntPtr lpSource, int dwMessageId,
            int dwLanguageId, StringBuilder lpBuffer, int nSize, IntPtr[] arguments);        
        [DllImport(ExternDll.Kernel32, CharSet=System.Runtime.InteropServices.CharSet.Auto, SetLastError=true)]
        public static extern int FormatMessage(int dwFlags, IntPtr lpSource, int dwMessageId,
            int dwLanguageId, StringBuilder lpBuffer, int nSize, IntPtr arguments);
        [DllImport(ExternDll.Kernel32, CharSet=System.Runtime.InteropServices.CharSet.Unicode, SetLastError=true)]
        public static extern int FormatMessage(int dwFlags, IntPtr source, int dwMessageId,
            int dwLanguageId, char[] msgBuffer, int nSize, int[] insertStringAddrs);

        [DllImport(ExternDll.Kernel32, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern void OutputDebugString(String message);
        [DllImport(ExternDll.User32, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern int MessageBox(IntPtr hWnd, string text, string caption, int type);
        [DllImport(ExternDll.Kernel32, SetLastError=true)]
        public static extern int WaitForSingleObject(IntPtr handle, int timeout);
        [DllImport(ExternDll.Kernel32, ExactSpelling=true, CharSet=System.Runtime.InteropServices.CharSet.Auto, SetLastError=true)]
        public static extern bool CloseHandle(IntPtr handle);

    }
}
