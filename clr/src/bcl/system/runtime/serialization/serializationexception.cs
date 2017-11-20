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
** Class: SerializationException
**
**                                
**
** Purpose: Thrown when something goes wrong during serialization or 
**          deserialization.
**
** Date: May 11, 1999
**
=============================================================================*/

namespace System.Runtime.Serialization {
    
	using System;
	using System.Runtime.Serialization;

    /// <include file='doc\SerializationException.uex' path='docs/doc[@for="SerializationException"]/*' />
    [Serializable] public class SerializationException : SystemException {
    	
    	private static String _nullMessage = Environment.GetResourceString("Arg_SerializationException");
    	
        // Creates a new SerializationException with its message 
        // string set to a default message.
        /// <include file='doc\SerializationException.uex' path='docs/doc[@for="SerializationException.SerializationException"]/*' />
        public SerializationException() 
            : base(_nullMessage) {
    		SetErrorCode(__HResults.COR_E_SERIALIZATION);
        }
    	
        /// <include file='doc\SerializationException.uex' path='docs/doc[@for="SerializationException.SerializationException1"]/*' />
        public SerializationException(String message) 
            : base(message) {
    		SetErrorCode(__HResults.COR_E_SERIALIZATION);
        }

        /// <include file='doc\SerializationException.uex' path='docs/doc[@for="SerializationException.SerializationException2"]/*' />
        public SerializationException(String message, Exception innerException) : base (message, innerException) {
    		SetErrorCode(__HResults.COR_E_SERIALIZATION);
        }

        /// <include file='doc\SerializationException.uex' path='docs/doc[@for="SerializationException.SerializationException3"]/*' />
        protected SerializationException(SerializationInfo info, StreamingContext context) : base (info, context) {
        }
    }
}
