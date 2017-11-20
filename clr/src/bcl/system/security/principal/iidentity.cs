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
//  IIdentity.cs
//
//  All identities will implement this interface
//

namespace System.Security.Principal
{
	using System.Runtime.Remoting;
	using System;
	using System.Security.Util;
    /// <include file='doc\IIdentity.uex' path='docs/doc[@for="IIdentity"]/*' />
    public interface IIdentity
    {
        /// <include file='doc\IIdentity.uex' path='docs/doc[@for="IIdentity.Name"]/*' />
        // Access to the name string
        String Name { get; }
        /// <include file='doc\IIdentity.uex' path='docs/doc[@for="IIdentity.AuthenticationType"]/*' />

        // Access to Authentication 'type' info
        String AuthenticationType { get; }
        /// <include file='doc\IIdentity.uex' path='docs/doc[@for="IIdentity.IsAuthenticated"]/*' />
        
        // Determine if this represents the unauthenticated identity
        bool IsAuthenticated { get; }
        
    }

}
