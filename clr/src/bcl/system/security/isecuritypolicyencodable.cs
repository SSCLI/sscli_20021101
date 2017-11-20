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
// ISecurityPolicyEncodable.cs
//
// All encodable security classes that support encoding need to
// implement this interface
//

namespace System.Security  {
    
    using System;
    using System.Security.Util;
    using System.Security.Policy;
    
    
    /// <include file='doc\ISecurityPolicyEncodable.uex' path='docs/doc[@for="ISecurityPolicyEncodable"]/*' />
    public interface ISecurityPolicyEncodable
    {
        /// <include file='doc\ISecurityPolicyEncodable.uex' path='docs/doc[@for="ISecurityPolicyEncodable.ToXml"]/*' />
        SecurityElement ToXml( PolicyLevel level );
        /// <include file='doc\ISecurityPolicyEncodable.uex' path='docs/doc[@for="ISecurityPolicyEncodable.FromXml"]/*' />
    
        void FromXml( SecurityElement e, PolicyLevel level );
    }

}


