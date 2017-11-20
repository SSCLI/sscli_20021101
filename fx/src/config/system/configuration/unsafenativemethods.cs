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

namespace System.Configuration {
    using System.Runtime.InteropServices;
    using System;
    using System.Security.Permissions;
    using System.Collections;
    using System.IO;
    using System.Text;

    [
    System.Security.SuppressUnmanagedCodeSecurityAttribute()
    ]
    internal class UnsafeNativeMethods {
        [DllImport(ExternDll.Kernel32, CharSet=CharSet.Auto)]
        public extern static int GetModuleFileName(int module, StringBuilder filename, int size);
        [DllImport(ExternDll.Kernel32, CharSet=CharSet.Auto)]
        public extern static int GetModuleHandle(String moduleName);
    }
}

