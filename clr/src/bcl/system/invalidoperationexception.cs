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
** Class: InvalidOperationException
**
**                                                    
**
** Purpose: Exception class for denoting an object was in a state that
** made calling a method illegal.
**
** Date: March 24, 1999
**
=============================================================================*/
namespace System {
    
	using System;
	using System.Runtime.Serialization;
    /// <include file='doc\InvalidOperationException.uex' path='docs/doc[@for="InvalidOperationException"]/*' />
    [Serializable()] public class InvalidOperationException : SystemException
    {
        /// <include file='doc\InvalidOperationException.uex' path='docs/doc[@for="InvalidOperationException.InvalidOperationException"]/*' />
        public InvalidOperationException() 
            : base(Environment.GetResourceString("Arg_InvalidOperationException")) {
    		SetErrorCode(__HResults.COR_E_INVALIDOPERATION);
        }
        
        /// <include file='doc\InvalidOperationException.uex' path='docs/doc[@for="InvalidOperationException.InvalidOperationException1"]/*' />
        public InvalidOperationException(String message) 
            : base(message) {
    		SetErrorCode(__HResults.COR_E_INVALIDOPERATION);
        }
    
    	/// <include file='doc\InvalidOperationException.uex' path='docs/doc[@for="InvalidOperationException.InvalidOperationException2"]/*' />
    	public InvalidOperationException(String message, Exception innerException) 
            : base(message, innerException) {
    		SetErrorCode(__HResults.COR_E_INVALIDOPERATION);
        }

        /// <include file='doc\InvalidOperationException.uex' path='docs/doc[@for="InvalidOperationException.InvalidOperationException3"]/*' />
        protected InvalidOperationException(SerializationInfo info, StreamingContext context) : base(info, context) {
        }

    }
}

