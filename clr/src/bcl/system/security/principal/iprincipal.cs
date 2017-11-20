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
//  IPrincipal.cs
//
//  All roles will implement this interface
//

namespace System.Security.Principal
{
	using System.Runtime.Remoting;
	using System;
	using System.Security.Util;
    /// <include file='doc\IPrincipal.uex' path='docs/doc[@for="IPrincipal"]/*' />
    public interface IPrincipal
    {
        /// <include file='doc\IPrincipal.uex' path='docs/doc[@for="IPrincipal.Identity"]/*' />
        // Retrieve the identity object
        IIdentity Identity { get; }
        /// <include file='doc\IPrincipal.uex' path='docs/doc[@for="IPrincipal.IsInRole"]/*' />
        
        // Perform a check for a specific role
        bool IsInRole( String role );
        
    }

}
