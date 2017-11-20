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
// ISecurityEncodable.cs
//
// All encodable security classes that support encoding need to
// implement this interface
//

namespace System.Security  {
    
    using System;
    using System.Security.Util;
    
    
    /// <include file='doc\ISecurityEncodable.uex' path='docs/doc[@for="ISecurityEncodable"]/*' />
    public interface ISecurityEncodable
    {
        /// <include file='doc\ISecurityEncodable.uex' path='docs/doc[@for="ISecurityEncodable.ToXml"]/*' />
        SecurityElement ToXml();
        /// <include file='doc\ISecurityEncodable.uex' path='docs/doc[@for="ISecurityEncodable.FromXml"]/*' />
    
        void FromXml( SecurityElement e );
    }

}


