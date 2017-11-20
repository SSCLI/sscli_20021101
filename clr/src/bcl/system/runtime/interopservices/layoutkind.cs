// ==++==
// 
//   
//    Copyright (c) 2002 Microsoft Corporation.  All rights reserved.
//   
//    The use and distribution terms for this software are contained in the file
//    named license.txt, which can be found in the root of this distribution.
//    By using this software in any fashion, you are agreeing to be bound by the
//    terms of this license.
//   
//    You must not remove this notice, or any other, from this software.
//   
// 
// ==--==
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
namespace System.Runtime.InteropServices {
	using System;
    /** Used in the StructLayoutAttribute class
     **/
    /// <include file='doc\LayoutKind.uex' path='docs/doc[@for="LayoutKind"]/*' />
    [Serializable()] public enum LayoutKind
    {
        /// <include file='doc\LayoutKind.uex' path='docs/doc[@for="LayoutKind.Sequential"]/*' />
        Sequential		= 0, // 0x00000008,
        /// <include file='doc\LayoutKind.uex' path='docs/doc[@for="LayoutKind.Explicit"]/*' />
        Explicit		= 2, // 0x00000010,
        /// <include file='doc\LayoutKind.uex' path='docs/doc[@for="LayoutKind.Auto"]/*' />
        Auto			= 3, // 0x00000000,
    }
}
