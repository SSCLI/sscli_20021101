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
** File:    AssemblyNameFlags
**
** Author:  
**
** Purpose: Flags controlling how an AssemblyName is used
**          during binding
**
** Date:    Sept 29, 1999
**
===========================================================*/
namespace System.Reflection {
    
    using System;
    /// <include file='doc\AssemblyNameFlags.uex' path='docs/doc[@for="AssemblyNameFlags"]/*' />
    [Serializable, FlagsAttribute()]
    public enum AssemblyNameFlags
    {
        /// <include file='doc\AssemblyNameFlags.uex' path='docs/doc[@for="AssemblyNameFlags.None"]/*' />
        None = 0,
    
        // Flag used to indicate that an assembly ref contains the full public key, not the compressed token.
        // Must match afPublicKey in CorHdr.h.
        /// <include file='doc\AssemblyNameFlags.uex' path='docs/doc[@for="AssemblyNameFlags.PublicKey"]/*' />
        PublicKey = 1,
    }
}
