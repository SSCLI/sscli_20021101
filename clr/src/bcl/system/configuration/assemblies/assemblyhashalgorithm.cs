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
** File:    AssemblyHashAlgorithm
**
**                                     
**
** Purpose: 
**
** Date:    June 4, 1999
**
===========================================================*/
namespace System.Configuration.Assemblies {
    
    using System;
    /// <include file='doc\AssemblyHashAlgorithm.uex' path='docs/doc[@for="AssemblyHashAlgorithm"]/*' />
    [Serializable]
    public enum AssemblyHashAlgorithm
    {
        /// <include file='doc\AssemblyHashAlgorithm.uex' path='docs/doc[@for="AssemblyHashAlgorithm.None"]/*' />
        None        = 0,
        /// <include file='doc\AssemblyHashAlgorithm.uex' path='docs/doc[@for="AssemblyHashAlgorithm.MD5"]/*' />
        MD5         = 0x8003,
        /// <include file='doc\AssemblyHashAlgorithm.uex' path='docs/doc[@for="AssemblyHashAlgorithm.SHA1"]/*' />
        SHA1        = 0x8004
    }
}
