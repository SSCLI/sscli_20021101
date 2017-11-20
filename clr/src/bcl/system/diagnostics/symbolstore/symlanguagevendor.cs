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
/*============================================================
**
** Class:  SymLanguageVendor
**
**                                               
**
** A class to hold public guids for language vendors.
**
** Date:  Tue Sep 07 13:05:53 1999
** 
===========================================================*/
namespace System.Diagnostics.SymbolStore {
    // Only statics, does not need to be marked with the serializable attribute
    using System;

    /// <include file='doc\SymLanguageVendor.uex' path='docs/doc[@for="SymLanguageVendor"]/*' />
    public class SymLanguageVendor
    {
        /// <include file='doc\SymLanguageVendor.uex' path='docs/doc[@for="SymLanguageVendor.Microsoft"]/*' />
        public static readonly Guid Microsoft = new Guid(unchecked((int)0x994b45c4), unchecked((short) 0xe6e9), 0x11d2, 0x90, 0x3f, 0x00, 0xc0, 0x4f, 0xa3, 0x02, 0xa1);
    }
}
