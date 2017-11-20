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
** Class: NullReferenceException
**
**                                             
**
** Purpose: Exception class for dereferencing a null reference.
**
** Date: March 17, 1998
**
=============================================================================*/

namespace System {   
    
	using System;
	using System.Runtime.Serialization;
    /// <include file='doc\NullReferenceException.uex' path='docs/doc[@for="NullReferenceException"]/*' />
    [Serializable()] public class NullReferenceException : SystemException {
        /// <include file='doc\NullReferenceException.uex' path='docs/doc[@for="NullReferenceException.NullReferenceException"]/*' />
        public NullReferenceException() 
            : base(Environment.GetResourceString("Arg_NullReferenceException")) {
    		SetErrorCode(__HResults.COR_E_NULLREFERENCE);
        }
    
        /// <include file='doc\NullReferenceException.uex' path='docs/doc[@for="NullReferenceException.NullReferenceException1"]/*' />
        public NullReferenceException(String message) 
            : base(message) {
    		SetErrorCode(__HResults.COR_E_NULLREFERENCE);
        }
    	
        /// <include file='doc\NullReferenceException.uex' path='docs/doc[@for="NullReferenceException.NullReferenceException2"]/*' />
        public NullReferenceException(String message, Exception innerException) 
            : base(message, innerException) {
    		SetErrorCode(__HResults.COR_E_NULLREFERENCE);
        }

        /// <include file='doc\NullReferenceException.uex' path='docs/doc[@for="NullReferenceException.NullReferenceException3"]/*' />
        protected NullReferenceException(SerializationInfo info, StreamingContext context) : base(info, context) {}

    }

}
