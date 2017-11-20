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
//+----------------------------------------------------------------------------
//
// Microsoft Windows
// File:        ILease.cs
//
// Contents:    Interface for Lease
//
// History:     1/5/00                                 Created
//
//+----------------------------------------------------------------------------

namespace System.Runtime.Remoting.Lifetime
{
    using System;
    using System.Security.Permissions;

    /// <include file='doc\ILease.uex' path='docs/doc[@for="ILease"]/*' />
    public interface ILease
    {
		/// <include file='doc\ILease.uex' path='docs/doc[@for="ILease.Register"]/*' />		
		[SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)] 		
		void Register(ISponsor obj, TimeSpan renewalTime);
		/// <include file='doc\ILease.uex' path='docs/doc[@for="ILease.Register1"]/*' />
		[SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)] 		
		void Register(ISponsor obj);
		/// <include file='doc\ILease.uex' path='docs/doc[@for="ILease.Unregister"]/*' />
		[SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)] 		
		void Unregister(ISponsor obj);
		/// <include file='doc\ILease.uex' path='docs/doc[@for="ILease.Renew"]/*' />
		[SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)] 		
		TimeSpan Renew(TimeSpan renewalTime);
		/// <include file='doc\ILease.uex' path='docs/doc[@for="ILease.RenewOnCallTime"]/*' />
		
		TimeSpan RenewOnCallTime 
		{
		    [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)] 				
		    get;
		    [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)] 		
		    set;
		}
		/// <include file='doc\ILease.uex' path='docs/doc[@for="ILease.SponsorshipTimeout"]/*' />
		TimeSpan SponsorshipTimeout
		{
		    [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)] 		
		    get;
		    [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)] 		
		    set;
		}
		/// <include file='doc\ILease.uex' path='docs/doc[@for="ILease.InitialLeaseTime"]/*' />
		TimeSpan InitialLeaseTime
		{
		    [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)] 		
		    get;
		    [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)] 		
		    set;
		}
		/// <include file='doc\ILease.uex' path='docs/doc[@for="ILease.CurrentLeaseTime"]/*' />
		TimeSpan CurrentLeaseTime 
		{
		    [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)] 		
		    get;
		}		
		/// <include file='doc\ILease.uex' path='docs/doc[@for="ILease.CurrentState"]/*' />
		LeaseState CurrentState 
	        {
		    [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)] 		
		    get;
		}
    }
}
