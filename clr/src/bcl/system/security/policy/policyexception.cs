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
//  PolicyException.cs
//
//  Use this class to throw a PolicyException
//

namespace System.Security.Policy {
    
	using System;
	using System.Runtime.Serialization;
    /// <include file='doc\PolicyException.uex' path='docs/doc[@for="PolicyException"]/*' />
    [Serializable]
    public class PolicyException : SystemException
    {
        /// <include file='doc\PolicyException.uex' path='docs/doc[@for="PolicyException.PolicyException"]/*' />
        public PolicyException()
        
            : base(Environment.GetResourceString( "Policy_Default" )) {
            HResult = __HResults.CORSEC_E_POLICY_EXCEPTION;
        }
    
        /// <include file='doc\PolicyException.uex' path='docs/doc[@for="PolicyException.PolicyException1"]/*' />
        public PolicyException(String message)
        
            : base(message) {
            HResult = __HResults.CORSEC_E_POLICY_EXCEPTION;
        }
        
        /// <include file='doc\PolicyException.uex' path='docs/doc[@for="PolicyException.PolicyException2"]/*' />
        public PolicyException(String message, Exception exception)
        
            : base(message, exception) {
            HResult = __HResults.CORSEC_E_POLICY_EXCEPTION;
        }

        /// <include file='doc\PolicyException.uex' path='docs/doc[@for="PolicyException.PolicyException3"]/*' />
        protected PolicyException(SerializationInfo info, StreamingContext context) : base (info, context) {}

        internal PolicyException(String message, int hresult) : base (message)
        {
            HResult = hresult;
        }

        internal PolicyException(String message, int hresult, Exception exception) : base (message, exception)
        {
            HResult = hresult;
        }

    }

}
