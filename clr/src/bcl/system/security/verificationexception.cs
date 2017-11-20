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
/****************************************************************************
 *
 * Class: VerificationException
 *
 * Purpose: The exception class for verification failures.
 *
 *                                     
 *
 ****************************************************************************/

namespace System.Security {
	using System.Security;
	using System;
	using System.Runtime.Serialization;
    /// <include file='doc\VerificationException.uex' path='docs/doc[@for="VerificationException"]/*' />
    [Serializable] public class VerificationException : SystemException {
        /// <include file='doc\VerificationException.uex' path='docs/doc[@for="VerificationException.VerificationException"]/*' />
        public VerificationException() 
            : base(Environment.GetResourceString("Verification_Exception")) {
    		SetErrorCode(__HResults.COR_E_VERIFICATION);
        }
    
        /// <include file='doc\VerificationException.uex' path='docs/doc[@for="VerificationException.VerificationException1"]/*' />
        public VerificationException(String message) 
            : base(message) {
    		SetErrorCode(__HResults.COR_E_VERIFICATION);
        }
    	
        /// <include file='doc\VerificationException.uex' path='docs/doc[@for="VerificationException.VerificationException2"]/*' />
        public VerificationException(String message, Exception innerException) 
            : base(message, innerException) {
    		SetErrorCode(__HResults.COR_E_VERIFICATION);
        }
        
        /// <include file='doc\VerificationException.uex' path='docs/doc[@for="VerificationException.VerificationException3"]/*' />
        protected VerificationException(SerializationInfo info, StreamingContext context) : base(info, context) {
        }
    }

}
