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
// AccessException
// Thrown when we try accessing a member that we cannot
// access, due to it being removed, private or something similar.
////////////////////////////////////////////////////////////////////////////////

namespace System {
    
	using System;
	using System.Runtime.Serialization;
    // The AccessException is thrown when trying to access a class
    // member fails.
    // 
    /// <include file='doc\AccessException.uex' path='docs/doc[@for="AccessException"]/*' />
    [Serializable()] public class AccessException : SystemException {
    	
        // Creates a new AccessException with its message string set to
        // the empty string, its HRESULT set to COR_E_MEMBERACCESS, 
        // and its ExceptionInfo reference set to null. 
    	/// <include file='doc\AccessException.uex' path='docs/doc[@for="AccessException.AccessException"]/*' />
    	public AccessException() 
            : base(Environment.GetResourceString("Arg_AccessException")) {
    		SetErrorCode(__HResults.COR_E_MEMBERACCESS);
        }
    	
        // Creates a new AccessException with its message string set to
        // message, its HRESULT set to COR_E_ACCESS, 
        // and its ExceptionInfo reference set to null. 
        // 
        /// <include file='doc\AccessException.uex' path='docs/doc[@for="AccessException.AccessException1"]/*' />
        public AccessException(String message) 
            : base(message) {
    		SetErrorCode(__HResults.COR_E_MEMBERACCESS);
        }
    	
        /// <include file='doc\AccessException.uex' path='docs/doc[@for="AccessException.AccessException2"]/*' />
        public AccessException(String message, Exception inner) 
            : base(message, inner) {
    		SetErrorCode(__HResults.COR_E_MEMBERACCESS);
        }

        /// <include file='doc\AccessException.uex' path='docs/doc[@for="AccessException.AccessException3"]/*' />
        protected AccessException(SerializationInfo info, StreamingContext context) : base(info, context) {
        }

    }
}
