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
/*=============================================================================
**
** Class: ExternalException
**
**                                                    
**
** Purpose: Exception base class for all errors from Interop or Structured 
**          Exception Handling code.
**
** Date: March 24, 1999
**
=============================================================================*/

namespace System.Runtime.InteropServices {

	using System;
	using System.Runtime.Serialization;
    // Base exception for COM Interop errors &; Structured Exception Handler
    // exceptions.
    // 
    /// <include file='doc\ExternalException.uex' path='docs/doc[@for="ExternalException"]/*' />
    [Serializable()] public class ExternalException : SystemException {
        /// <include file='doc\ExternalException.uex' path='docs/doc[@for="ExternalException.ExternalException"]/*' />
        public ExternalException() 
            : base(Environment.GetResourceString("Arg_ExternalException")) {
    		SetErrorCode(__HResults.E_FAIL);
        }
    	
        /// <include file='doc\ExternalException.uex' path='docs/doc[@for="ExternalException.ExternalException1"]/*' />
        public ExternalException(String message) 
            : base(message) {
    		SetErrorCode(__HResults.E_FAIL);
        }
    	
        /// <include file='doc\ExternalException.uex' path='docs/doc[@for="ExternalException.ExternalException2"]/*' />
        public ExternalException(String message, Exception inner) 
            : base(message, inner) {
    		SetErrorCode(__HResults.E_FAIL);
        }

		/// <include file='doc\ExternalException.uex' path='docs/doc[@for="ExternalException.ExternalException3"]/*' />
		public ExternalException(String message,int errorCode) 
            : base(message) {
    		SetErrorCode(errorCode);
        }

        /// <include file='doc\ExternalException.uex' path='docs/doc[@for="ExternalException.ExternalException4"]/*' />
        protected ExternalException(SerializationInfo info, StreamingContext context) : base(info, context) {
        }

		/// <include file='doc\ExternalException.uex' path='docs/doc[@for="ExternalException.ErrorCode"]/*' />
		public virtual int ErrorCode {
    		get { return HResult; }
        }
    }
}
