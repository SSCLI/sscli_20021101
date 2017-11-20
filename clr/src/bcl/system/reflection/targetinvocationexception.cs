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
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
// TargetInvocationException is used to report an exception that was thrown
//	by the target of an invocation.
//
// Date: March 98
//
namespace System.Reflection {
    
    
	using System;
	using System.Runtime.Serialization;
    /// <include file='doc\TargetInvocationException.uex' path='docs/doc[@for="TargetInvocationException"]/*' />
	[Serializable()] 
    public sealed class TargetInvocationException : ApplicationException {

		// This exception is not creatable without specifying the
		//	inner exception.
    	private TargetInvocationException()
	        : base(Environment.GetResourceString("Arg_TargetInvocationException")) {
    		SetErrorCode(__HResults.COR_E_TARGETINVOCATION);
    	}

		// This is called from within the runtime.
        private TargetInvocationException(String message) : base(message) {
    		SetErrorCode(__HResults.COR_E_TARGETINVOCATION);
        }   	
    	
        /// <include file='doc\TargetInvocationException.uex' path='docs/doc[@for="TargetInvocationException.TargetInvocationException"]/*' />
        public TargetInvocationException(System.Exception inner) 
			: base(Environment.GetResourceString("Arg_TargetInvocationException"), inner) {
    		SetErrorCode(__HResults.COR_E_TARGETINVOCATION);
        }
    
        /// <include file='doc\TargetInvocationException.uex' path='docs/doc[@for="TargetInvocationException.TargetInvocationException1"]/*' />
        public TargetInvocationException(String message, Exception inner) : base(message, inner) {
    		SetErrorCode(__HResults.COR_E_TARGETINVOCATION);
        }

        internal TargetInvocationException(SerializationInfo info, StreamingContext context) : base (info, context) {
        }
    }
}
