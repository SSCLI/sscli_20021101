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
//  IMembershipCondition.cs
//
//  Interface that all MembershipConditions must implement
//

namespace System.Security.Policy {
    
	using System;
    /// <include file='doc\IMembershipCondition.uex' path='docs/doc[@for="IMembershipCondition"]/*' />
    public interface IMembershipCondition : ISecurityEncodable, ISecurityPolicyEncodable
    {
        /// <include file='doc\IMembershipCondition.uex' path='docs/doc[@for="IMembershipCondition.Check"]/*' />
        bool Check( Evidence evidence );
        /// <include file='doc\IMembershipCondition.uex' path='docs/doc[@for="IMembershipCondition.Copy"]/*' />
    
        IMembershipCondition Copy();
        /// <include file='doc\IMembershipCondition.uex' path='docs/doc[@for="IMembershipCondition.ToString"]/*' />
        
        String ToString();
        /// <include file='doc\IMembershipCondition.uex' path='docs/doc[@for="IMembershipCondition.Equals"]/*' />

        bool Equals( Object obj );
        
    }
}
