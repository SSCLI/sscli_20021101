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
// IStackWalk.cs
//

namespace System.Security
{

	/// <include file='doc\IStackWalk.uex' path='docs/doc[@for="IStackWalk"]/*' />
	public interface IStackWalk
	{
        /// <include file='doc\IStackWalk.uex' path='docs/doc[@for="IStackWalk.Assert"]/*' />
        [DynamicSecurityMethodAttribute()]
        void Assert();
        /// <include file='doc\IStackWalk.uex' path='docs/doc[@for="IStackWalk.Demand"]/*' />
        
        void Demand();
        /// <include file='doc\IStackWalk.uex' path='docs/doc[@for="IStackWalk.Deny"]/*' />
        
        [DynamicSecurityMethodAttribute()]
        void Deny();
        /// <include file='doc\IStackWalk.uex' path='docs/doc[@for="IStackWalk.PermitOnly"]/*' />
        
        [DynamicSecurityMethodAttribute()]
        void PermitOnly();
	}
}
