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
/*============================================================
**
** Class:  FormatException
**
**                                        
**
** Purpose: Exception to designate an illegal argument to FormatMessage.
**
** Date:  February 10, 1998
** 
===========================================================*/
namespace System {
    
	using System;
	using System.Runtime.Serialization;
    /// <include file='doc\FormatException.uex' path='docs/doc[@for="FormatException"]/*' />
    [Serializable()] public class FormatException : SystemException {
        /// <include file='doc\FormatException.uex' path='docs/doc[@for="FormatException.FormatException"]/*' />
        public FormatException() 
            : base(Environment.GetResourceString("Arg_FormatException")) {
    		SetErrorCode(__HResults.COR_E_FORMAT);
        }
    
        /// <include file='doc\FormatException.uex' path='docs/doc[@for="FormatException.FormatException1"]/*' />
        public FormatException(String message) 
            : base(message) {
    		SetErrorCode(__HResults.COR_E_FORMAT);
        }
    	
        /// <include file='doc\FormatException.uex' path='docs/doc[@for="FormatException.FormatException2"]/*' />
        public FormatException(String message, Exception innerException) 
            : base(message, innerException) {
    		SetErrorCode(__HResults.COR_E_FORMAT);
        }

        /// <include file='doc\FormatException.uex' path='docs/doc[@for="FormatException.FormatException3"]/*' />
        protected FormatException(SerializationInfo info, StreamingContext context) : base(info, context) {
        }

    }

}
