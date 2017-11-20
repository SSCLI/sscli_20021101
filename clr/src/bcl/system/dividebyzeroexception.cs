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
** Class: DivideByZeroException
**
**                                             
**
** Purpose: Exception class for bad arithmetic conditions!
**
** Date: March 17, 1998
**
=============================================================================*/

namespace System {
    
	using System;
	using System.Runtime.Serialization;
    /// <include file='doc\DivideByZeroException.uex' path='docs/doc[@for="DivideByZeroException"]/*' />
    [Serializable()] public class DivideByZeroException : ArithmeticException {
        /// <include file='doc\DivideByZeroException.uex' path='docs/doc[@for="DivideByZeroException.DivideByZeroException"]/*' />
        public DivideByZeroException() 
            : base(Environment.GetResourceString("Arg_DivideByZero")) {
    		SetErrorCode(__HResults.COR_E_DIVIDEBYZERO);
        }
    
        /// <include file='doc\DivideByZeroException.uex' path='docs/doc[@for="DivideByZeroException.DivideByZeroException1"]/*' />
        public DivideByZeroException(String message) 
            : base(message) {
    		SetErrorCode(__HResults.COR_E_DIVIDEBYZERO);
        }
    
        /// <include file='doc\DivideByZeroException.uex' path='docs/doc[@for="DivideByZeroException.DivideByZeroException2"]/*' />
        public DivideByZeroException(String message, Exception innerException) 
            : base(message, innerException) {
    		SetErrorCode(__HResults.COR_E_DIVIDEBYZERO);
        }

        /// <include file='doc\DivideByZeroException.uex' path='docs/doc[@for="DivideByZeroException.DivideByZeroException3"]/*' />
        protected DivideByZeroException(SerializationInfo info, StreamingContext context) : base(info, context) {
        }
    }
}
