//------------------------------------------------------------------------------
// <copyright file="NativeMethods.cs" company="Microsoft">
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
    using System;
    using System.Text;
    using System.Threading;
    using System.Runtime.Remoting;
    using System.Runtime.InteropServices;
    using System.Diagnostics;
    using System.ComponentModel;

    // not public!
    internal sealed class NativeMethods {

        public static readonly IntPtr INVALID_HANDLE_VALUE = (IntPtr)(-1);

        public const int GENERIC_READ = unchecked(((int)0x80000000));
        public const int GENERIC_WRITE = (0x40000000);

        public const int FILE_SHARE_READ = 0x00000001;
        public const int FILE_SHARE_WRITE = 0x00000002;
        public const int FILE_SHARE_DELETE = 0x00000004;

        public const int CREATE_ALWAYS = 2;

        public const int FILE_ATTRIBUTE_NORMAL = 0x00000080;

        public const int STARTF_USESTDHANDLES = 0x00000100;

        public const int STD_INPUT_HANDLE = -10;
        public const int STD_OUTPUT_HANDLE = -11;
        public const int STD_ERROR_HANDLE = -12;

        public const int STILL_ACTIVE = 0x00000103;
        public const int SW_HIDE = 0;

        public const int WAIT_OBJECT_0    = 0x00000000;
        public const int WAIT_FAILED      = unchecked((int)0xFFFFFFFF);
        public const int WAIT_TIMEOUT     = 0x00000102;
        public const int WAIT_ABANDONED   = 0x00000080;
        public const int WAIT_ABANDONED_0 = WAIT_ABANDONED;


        [StructLayout(LayoutKind.Sequential)]
        public class SECURITY_ATTRIBUTES {
            public int nLength = Marshal.SizeOf(typeof(SECURITY_ATTRIBUTES));
            public IntPtr lpSecurityDescriptor = IntPtr.Zero;
            public bool bInheritHandle = false;
        }

        [StructLayout(LayoutKind.Sequential)]
        public class STARTUPINFO {
            public int cb;
            public IntPtr lpReserved = IntPtr.Zero;
            public IntPtr lpDesktop = IntPtr.Zero;
            public IntPtr lpTitle = IntPtr.Zero;
            public int dwX = 0;
            public int dwY = 0;
            public int dwXSize = 0;
            public int dwYSize = 0;
            public int dwXCountChars = 0;
            public int dwYCountChars = 0;
            public int dwFillAttribute = 0;
            public int dwFlags = 0;
            public short wShowWindow = 0;
            public short cbReserved2 = 0;
            public IntPtr lpReserved2 = IntPtr.Zero;
            public IntPtr hStdInput = IntPtr.Zero;
            public IntPtr hStdOutput = IntPtr.Zero;
            public IntPtr hStdError = IntPtr.Zero;
        }

        [StructLayout(LayoutKind.Sequential)]
        public class PROCESS_INFORMATION {
            public IntPtr   hProcess = IntPtr.Zero;
            public IntPtr   hThread = IntPtr.Zero;
            public int      dwProcessId = 0;
            public int      dwThreadId = 0;
        }

        [DllImport(ExternDll.Kernel32, CharSet=System.Runtime.InteropServices.CharSet.Auto, SetLastError=true)]
        public static extern bool GetExitCodeProcess(IntPtr processHandle, out int exitCode);
        [DllImport(ExternDll.Kernel32, CharSet=System.Runtime.InteropServices.CharSet.Auto, SetLastError=true)]
        public static extern bool GetProcessTimes(IntPtr handle, long[] creation, long[] exit, long[] kernel, long[] user);

        [DllImport(ExternDll.Kernel32, CharSet=System.Runtime.InteropServices.CharSet.Ansi, SetLastError=true)]
        public static extern IntPtr GetStdHandle(int whichHandle);
        [DllImport(ExternDll.Kernel32, CharSet=System.Runtime.InteropServices.CharSet.Auto, SetLastError=true)]
        public static extern int GetConsoleCP();
        [DllImport(ExternDll.Kernel32, CharSet=System.Runtime.InteropServices.CharSet.Auto, SetLastError=true)]
        public static extern int GetConsoleOutputCP();

        [DllImport(ExternDll.Kernel32, ExactSpelling=true, SetLastError=true)]
        public static extern int WaitForSingleObject(IntPtr handle, int timeout);

        [StructLayout(LayoutKind.Sequential)]
        public class SecurityAttributes {
            public int nLength = Marshal.SizeOf(typeof(SecurityAttributes));
            public IntPtr lpSecurityDescriptor;
            public bool bInheritHandle;
        }

        [DllImport(ExternDll.Kernel32, CharSet=System.Runtime.InteropServices.CharSet.Auto, SetLastError=true)]
        public static extern bool CreatePipe(out IntPtr hReadPipe, out IntPtr hWritePipe, SecurityAttributes lpPipeAttributes, int nSize);
        public static void IntCreatePipe(out IntPtr hReadPipe, out IntPtr hWritePipe, SecurityAttributes lpPipeAttributes, int nSize) {
            bool ret = CreatePipe(out hReadPipe, out hWritePipe, lpPipeAttributes, nSize);
            if (!ret || hReadPipe == INVALID_HANDLE_VALUE || hWritePipe == INVALID_HANDLE_VALUE) 
                throw new Win32Exception();
        }

        [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Auto)]
        public class CreateProcessStartupInfo 
        {
            public int cb = Marshal.SizeOf(typeof(CreateProcessStartupInfo));
            public string lpReserved = null;
            public string lpDesktop = null;
            public string lpTitle = null;
            public int dwX = 0;
            public int dwY = 0;
            public int dwXSize = 0;
            public int dwYSize = 0;
            public int dwXCountChars = 0;
            public int dwYCountChars = 0;
            public int dwFillAttribute = 0;
            public int dwFlags = 0;
            public short wShowWindow = 0;
            public short cbReserved2 = 0;
            public IntPtr lpReserved2 = (IntPtr)0;
            public IntPtr hStdInput = (IntPtr)0;
            public IntPtr hStdOutput = (IntPtr)0;
            public IntPtr hStdError = (IntPtr)0;
        }

        [StructLayout(LayoutKind.Sequential)]
        public class CreateProcessProcessInformation 
        {
            public IntPtr hProcess = (IntPtr)0;
            public IntPtr hThread = (IntPtr)0;
            public int dwProcessId = 0;
            public int dwThreadId = 0;
        }

        [DllImport(ExternDll.Kernel32, CharSet=System.Runtime.InteropServices.CharSet.Auto, SetLastError=true)]
        public static extern bool CreateProcess(
            string lpApplicationName,                   // LPCTSTR
            [In] StringBuilder lpCommandLine,           // LPTSTR - note: CreateProcess might insert a null somewhere in this string
            SecurityAttributes lpProcessAttributes,     // LPSECURITY_ATTRIBUTES
            SecurityAttributes lpThreadAttributes,      // LPSECURITY_ATTRIBUTES
            bool bInheritHandles,                       // BOOL
            int dwCreationFlags,                        // DWORD
            IntPtr lpEnvironment,                       // LPVOID
            string lpCurrentDirectory,                  // LPCTSTR
            CreateProcessStartupInfo lpStartupInfo,                 // LPSTARTUPINFO
            CreateProcessProcessInformation lpProcessInformation    // LPPROCESS_INFORMATION
            );

        [DllImport(ExternDll.Kernel32, CharSet=System.Runtime.InteropServices.CharSet.Auto, SetLastError=true)]
        public static extern bool TerminateProcess(IntPtr processHandle, int exitCode);

        [DllImport(ExternDll.Kernel32, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern int GetCurrentProcessId();
        [DllImport(ExternDll.Kernel32, CharSet=System.Runtime.InteropServices.CharSet.Ansi, SetLastError=true)]
        public static extern IntPtr GetCurrentProcess();
        
              
    }
}

