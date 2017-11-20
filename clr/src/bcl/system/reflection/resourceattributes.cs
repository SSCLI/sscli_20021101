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
//
// ResourceAttributes is an enum which defines the attributes that may be associated
//  with a manifest resource.  The values here are defined in Corhdr.h.
//
// Date: April 2000
//
namespace System.Reflection {
    
    using System;
    /// <include file='doc\ResourceAttributes.uex' path='docs/doc[@for="ResourceAttributes"]/*' />
    [Serializable, Flags]  
    public enum ResourceAttributes
    {
        /// <include file='doc\ResourceAttributes.uex' path='docs/doc[@for="ResourceAttributes.Public"]/*' />
        Public          =   0x0001,
        /// <include file='doc\ResourceAttributes.uex' path='docs/doc[@for="ResourceAttributes.Private"]/*' />
        Private         =   0x0002,
    }
}
