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
** Class: OverflowException
**
**                                        
**
** Purpose: Exception class for Arthimatic Overflows.
**
** Date: August 31, 1998
**
=============================================================================*/

namespace System {
 
    
	using System;
	using System.Runtime.Serialization;
    /// <include file='doc\OverflowException.uex' path='docs/doc[@for="OverflowException"]/*' />
    [Serializable()] public class OverflowException : ArithmeticException {
        /// <include file='doc\OverflowException.uex' path='docs/doc[@for="OverflowException.OverflowException"]/*' />
        public OverflowException() 
            : base(Environment.GetResourceString("Arg_OverflowException")) {
    		SetErrorCode(__HResults.COR_E_OVERFLOW);
        }
    
        /// <include file='doc\OverflowException.uex' path='docs/doc[@for="OverflowException.OverflowException1"]/*' />
        public OverflowException(String message) 
            : base(message) {
    		SetErrorCode(__HResults.COR_E_OVERFLOW);
        }
    	
        /// <include file='doc\OverflowException.uex' path='docs/doc[@for="OverflowException.OverflowException2"]/*' />
        public OverflowException(String message, Exception innerException) 
            : base(message, innerException) {
    		SetErrorCode(__HResults.COR_E_OVERFLOW);
        }

        /// <include file='doc\OverflowException.uex' path='docs/doc[@for="OverflowException.OverflowException3"]/*' />
        protected OverflowException(SerializationInfo info, StreamingContext context) : base(info, context) {
        }

    }

}
