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
//  PermissionState.cs
//
//  The Runtime policy manager.  Maintains a set of IdentityMapper objects that map 
//  inbound evidence to groups.  Resolves an identity into a set of permissions
//

namespace System.Security.Permissions {
    
	using System;
    /// <include file='doc\PermissionState.uex' path='docs/doc[@for="PermissionState"]/*' />
	[Serializable]
    public enum PermissionState
    {
        /// <include file='doc\PermissionState.uex' path='docs/doc[@for="PermissionState.Unrestricted"]/*' />
        Unrestricted = 1,
        /// <include file='doc\PermissionState.uex' path='docs/doc[@for="PermissionState.None"]/*' />
        None = 0,
    } 
    
}
