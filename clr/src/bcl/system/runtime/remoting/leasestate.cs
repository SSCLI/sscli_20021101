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
// File:        LeaseState.cs
//
// Contents:    Lease States
//
// History:     1/5/00                                 Created
//
//+----------------------------------------------------------------------------

namespace System.Runtime.Remoting.Lifetime
{
    using System;

  /// <include file='doc\LeaseState.uex' path='docs/doc[@for="LeaseState"]/*' />
  [Serializable]
  public enum LeaseState
    {
		/// <include file='doc\LeaseState.uex' path='docs/doc[@for="LeaseState.Null"]/*' />
		Null = 0,
		/// <include file='doc\LeaseState.uex' path='docs/doc[@for="LeaseState.Initial"]/*' />
		Initial = 1,
		/// <include file='doc\LeaseState.uex' path='docs/doc[@for="LeaseState.Active"]/*' />
		Active = 2,
		/// <include file='doc\LeaseState.uex' path='docs/doc[@for="LeaseState.Renewing"]/*' />
		Renewing = 3,
		/// <include file='doc\LeaseState.uex' path='docs/doc[@for="LeaseState.Expired"]/*' />
		Expired = 4,
    }
} 
