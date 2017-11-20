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
//  IIdentityPermissionFactory.cs
//
//  All Identities will implement this interface.
//

namespace System.Security.Policy {
	using System.Runtime.Remoting;
	using System;
	using System.Security.Util;
    /// <include file='doc\IIdentityPermissionFactory.uex' path='docs/doc[@for="IIdentityPermissionFactory"]/*' />
    public interface IIdentityPermissionFactory
    {
        /// <include file='doc\IIdentityPermissionFactory.uex' path='docs/doc[@for="IIdentityPermissionFactory.CreateIdentityPermission"]/*' />
        IPermission CreateIdentityPermission( Evidence evidence );
    }

}
